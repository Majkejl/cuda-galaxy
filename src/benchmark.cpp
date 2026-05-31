
#include "benchmark.h"

#include <fstream>
#include <iostream>

Benchmark::Benchmark() : m_sim() {
    m_sim.mallocPos();
}

void Benchmark::run(int iters) {
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    for (int s = 0; s < 50; ++s) { // warm-up
        m_sim.step(BENCHMARK_STEP);
    }
    cudaEventRecord(start);
    for (int s = 0; s < iters; ++s) {
        m_sim.step(BENCHMARK_STEP);
    }
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float ms = 0.f;
    cudaEventElapsedTime(&ms, start, stop);
    double msPerStep = ms / iters;

    double gflops = (20. * m_sim.particleCount() * m_sim.particleCount()) /
                    (msPerStep / 1000) / 1e9;
    std::cout << "GFLOPPS/s: " << gflops << '\n';

    // Append one row to the running results table (header: kernel, N, GFLOPS/s).
    // Kernel name is hardcoded per branch — this is the tiled-kernel branch.
    std::ofstream csv("performance.csv", std::ios::app);
    csv << "naive, " << m_sim.particleCount() << ", " << gflops << ", " << msPerStep << '\n';
}