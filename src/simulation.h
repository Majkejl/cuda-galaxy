#pragma once
#include <vector>
#include <cuda_runtime.h>   // float4, cudaGraphicsResource_t, and the host runtime API

// Host-side handle to the GPU simulation. The whole target already links the CUDA
// runtime (CUDA::cudart), so we now expose typed device pointers directly instead of
// hiding them behind void* — simpler and type-safe. The CUDA kernels still live in the
// .cu files; this class only issues host-side runtime/interop calls, so it compiles as
// a plain C++ translation unit (simulation.cpp).
constexpr bool BENCHMARK_STEP = true;

class Simulation {
public:
    Simulation();    // allocate device buffers + build initial conditions
    ~Simulation();   // free device buffers + unregister the interop resource

    Simulation(const Simulation&)            = delete;   // owns raw device memory
    Simulation& operator=(const Simulation&) = delete;

    void step(const bool benchmark = false);                          // advance the simulation by one timestep
    void registerVBO(unsigned int vbo);   // register the GL VBO (by name) for CUDA-GL interop
    void mallocPos();

    int particleCount() const;
    const void* initialPositions() const { return m_hInitPos.data(); }

    // Star-mass tint range, derived from the star distribution (mean ± 2*sigma). Used both for
    // the color ramp and as the size clamp; cores fall outside it and saturate to blue.
    float starMassMin() const { return m_colorMassMin; }
    float starMassMax() const { return m_colorMassMax; }
private:
    std::vector<float>     m_hInitPos;             // initial positions, handed to the renderer's VBO
    float                  m_colorMassMin = 0.f;   // mean - 2*sigma of the star mass distribution
    float                  m_colorMassMax = 1.f;   // mean + 2*sigma (also the point-size mass clamp)
    cudaGraphicsResource_t m_res     = nullptr;    // CUDA handle to the GL VBO (the live positions)
    float4*                m_dPos    = nullptr;    // device positions
    float4*                m_dVel    = nullptr;    // device velocities
    float4*                m_dForces = nullptr;    // device force/accel scratch
    int                    m_step    = 0;
};
