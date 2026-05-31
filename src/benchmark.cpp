
#include "benchmark.h"

#include <iostream>

Benchmark::Benchmark() : m_sim() {
    m_sim.mallocPos();
}

void Benchmark::run() {
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    for (int s = 0; s < 50; ++s) { // warm-up
        m_sim.step(BENCHMARK_STEP);
    }
    cudaEventRecord(start);
    for (int s = 0; s < ITERS; ++s) {
        m_sim.step(BENCHMARK_STEP);
    }
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float ms = 0.f;
    cudaEventElapsedTime(&ms, start, stop);
    double msPerStep = ms / ITERS;

    double gflops = (20. * m_sim.particleCount() * m_sim.particleCount()) /
                    (msPerStep / 1000) / 1e9;
    std::cout << "GFLOPS/s: " << gflops << '\n';
}