#include "simulation.h"
#include "nbody.cuh"

#include <vector>
#include <ranges>
#include <random>
#include <cuda_gl_interop.h>

Simulation::Simulation() {
    // cudaMalloc takes void** — m_d* are already void*, so &m_d* matches directly.
    CUDA_CHECK(cudaMalloc(&m_dPos,    N * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&m_dVel,    N * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&m_dForces, N * sizeof(float4)));

    // Placeholder random ICs — replaced by the two-galaxy generator on Day 4.
    std::vector<float4> hPos, hVel;
    hPos.reserve(N);
    hVel.reserve(N);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    for (int i : std::views::iota(0, N)) {
        hPos.emplace_back(dist(rng), dist(rng), dist(rng), 1.f);   // .w = mass
        hVel.emplace_back(0.f, 0.f, 0.f, 0.f);
    }
    m_hInitPos.resize(N * 4);
    std::memcpy(m_hInitPos.data(), hPos.data(), N * sizeof(float4));

    CUDA_CHECK(cudaMemcpy(m_dPos, hPos.data(), N * sizeof(float4), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(m_dVel, hVel.data(), N * sizeof(float4), cudaMemcpyHostToDevice));
}

Simulation::~Simulation() {
    cudaFree(m_dPos);
    cudaFree(m_dVel);
    cudaFree(m_dForces);
    CUDA_CHECK(cudaGraphicsUnregisterResource(static_cast<cudaGraphicsResource_t>(m_res)));
}

void registerVBO(unsigned vbo) {
    cudaGraphicsResource_t res = nullptr;
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(&res, vbo, cudaGraphicsRegisterFlagsNone));
    m_res = res;
}

int Simulation::particleCount() const { return N; }

void Simulation::step() {
    auto res = static_cast<cudaGraphicsResource_t>(m_res);
    CUDA_CHECK(cudaGraphicsMapResources(1, &m_res));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(&static_cast<float4*>m_dPos, N * sizeof(float4), m_res));
    stepSimulation(static_cast<float4*>(m_dPos),
                   static_cast<float4*>(m_dVel),
                   static_cast<float4*>(m_dForces));
    CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_res));

#ifdef NBODY_DEBUG
    if (m_step % 100 == 0) {
        std::vector<float4> hPos(N), hVel(N);
        CUDA_CHECK(cudaMemcpy(hPos.data(), m_dPos, N * sizeof(float4), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(hVel.data(), m_dVel, N * sizeof(float4), cudaMemcpyDeviceToHost));
        printEnergyStats(hPos, hVel);
    }
#endif
    ++m_step;
}
