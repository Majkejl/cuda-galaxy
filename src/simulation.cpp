#include "simulation.h"
#include "nbody.cuh"
#include "galaxy.h"

#include <vector>
#include <ranges>
#include <random>
#include <cstring>             // std::memcpy

// cuda_gl_interop.h pulls in <GL/gl.h>, whose declarations use the WINGDIAPI/APIENTRY
// macros — which only exist once <windows.h> has been included. So on Windows that must
// come first or GL/gl.h fails to parse under MSVC. (LEAN_AND_MEAN/NOMINMAX keep the
// windows.h blast radius small; GDI — and thus WINGDIAPI — is still pulled in.)
#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <cuda_gl_interop.h>   // cudaGraphics{GLRegisterBuffer,MapResources,...} — host-side interop API

Simulation::Simulation() {
    // Velocities and the force scratch buffer are CUDA-only (never rendered), so they get
    // their own device allocations. Positions instead live in the VBO: the renderer uploads
    // the initial positions, and from then on CUDA maps that same buffer each frame (see
    // registerVBO()/step()). So there is deliberately NO separate device position buffer here.
    //
    // &m_dVel is a float4**, which does NOT implicitly convert to void** in C++ — hence the
    // reinterpret_cast idiom every cudaMalloc needs now that the members are typed pointers.
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&m_dVel),    N * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&m_dForces), N * sizeof(float4)));

    std::vector<float4> hPos, hVel;
    hPos.reserve(N);
    hVel.reserve(N);
   
    GalaxyParams init(N / 2); // count
    init.radius = 0.7f;
    init.pos = {-2.5f, -0.3f, 0.f};
    init.vel = {1.2f, 0.f, 0.f};
    init.tiltRadians = 0.5f;
    //init.uniform = true;
    makeGalaxy(init, hPos, hVel);

    init.pos = {2.5f, 0.3f, 0.f};
    init.vel = {-1.2f, 0.f, 0.f};
    init.tiltRadians = -1.f;
    makeGalaxy(init, hPos, hVel);

    // Star tint/size range from the generative distribution: makeGalaxy draws star masses from
    // N(starMass, starMass*randomness), so mean = starMass and sigma = starMass*randomness. Both
    // galaxies reuse the same `init` params, so one range covers them. Normalizing over mean ±
    // k*sigma derives the bounds from the population itself — the cores (coreMass, not drawn from
    // this distribution) sit far above mean + k*sigma and are excluded as outliers by construction.
    constexpr float kColorSigmas = 2.f;   // std-devs spanning the yellow→blue ramp (tunable)
    const float mean  = init.starMass;
    const float sigma = init.starMass * init.randomness;
    m_colorMassMin = mean - kColorSigmas * sigma;
    m_colorMassMax = mean + kColorSigmas * sigma;

    // Hand the initial positions to the renderer, which uploads them into the VBO.
    m_hInitPos.resize(N * 4);
    std::memcpy(m_hInitPos.data(), hPos.data(), N * sizeof(float4));

    // Only velocities need a device upload here; positions reach the GPU via the VBO.
    CUDA_CHECK(cudaMemcpy(m_dVel, hVel.data(), N * sizeof(float4), cudaMemcpyHostToDevice));
}

Simulation::~Simulation() {
    if (m_res) cudaGraphicsUnregisterResource(m_res);   // release the GL-shared buffer (GL ctx still alive here)
    cudaFree(m_dPos);
    cudaFree(m_dVel);
    cudaFree(m_dForces);
}

void Simulation::registerVBO(unsigned int vbo) {
    // One-time, expensive call: declares that this GL buffer will be shared with CUDA.
    // FlagsNone = read+write, contents preserved across frames. NOT WriteDiscard: every
    // step computeForces READS last frame's positions out of this very buffer before
    // integrate overwrites them, so the prior contents must survive.
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&m_res, vbo, cudaGraphicsRegisterFlagsNone));
}

void Simulation::mallocPos() {
    CUDA_CHECK(cudaMalloc(&m_dPos, N * sizeof(float4)));
    CUDA_CHECK(cudaMemcpy(m_dPos, m_hInitPos.data(), N * sizeof(float4), cudaMemcpyHostToDevice));
}

int Simulation::particleCount() const { return N; }

void Simulation::step(const bool benchmark) {
    
    float4* dPos = m_dPos;
    if (!benchmark) {

        // Producer/consumer handoff. Map gives the buffer to CUDA (GL must not touch it now);
        // unmap gives it back to GL for the draw. Map -> kernels -> unmap are all issued on the
        // default stream in order, so no explicit cudaDeviceSynchronize is needed for the handoff.
        CUDA_CHECK(cudaGraphicsMapResources(1, &m_res));
        
        // The mapped pointer aliases the VBO's bytes as float4 (x,y,z, w=mass). Valid only
        // between map and unmap, so it's a local — never cached across frames.
        size_t  bytes = 0;
        CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(reinterpret_cast<void**>(&dPos), &bytes, m_res));
    }

    stepSimulation(dPos, m_dVel, m_dForces);

#ifdef NBODY_DEBUG
    if (m_step % 100 == 0) {
        // Read positions straight from the mapped VBO — must happen before unmap.
        std::vector<float4> hPos(N), hVel(N);
        CUDA_CHECK(cudaMemcpy(hPos.data(), dPos, N * sizeof(float4), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(hVel.data(), m_dVel, N * sizeof(float4), cudaMemcpyDeviceToHost));
        printEnergyStats(hPos, hVel);
    }
#endif

    if (!benchmark) CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_res));
    ++m_step;
}
