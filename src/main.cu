#include <vector>
#include <ranges>
#include <random>
#include <iostream>
#include "nbody.cuh"

int main(int /*argc*/, char* /*argv*/[]) {
    
    // buffer initialization
    float4 *d_pos, *d_vel, *d_forces;
    CUDA_CHECK(cudaMalloc(&d_pos, N * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_vel, N * sizeof(float4)));
    CUDA_CHECK(cudaMalloc(&d_forces, N * sizeof(float4)));

    std::vector<float4> h_pos, h_vel;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float>  dist(-1.f, 1.f);
    for (int i : std::views::iota(0, N)) {
        h_pos.emplace_back(dist(rng), dist(rng), dist(rng), 1.f);
        h_vel.emplace_back(0.f, 0.f, 0.f, 0.f);
    };
    
    CUDA_CHECK(cudaMemcpy(d_pos, h_pos.data(), N * sizeof(float4), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_vel, h_vel.data(), N * sizeof(float4), cudaMemcpyHostToDevice));


    // simulation steps
    for (int i : std::views::iota(0, 2000)) {
        stepSimulation(d_pos, d_vel, d_forces);
#ifdef NBODY_DEBUG
        if (i % 100 == 0) {
            CUDA_CHECK(cudaMemcpy(h_pos.data(), d_pos, N * sizeof(float4), cudaMemcpyDeviceToHost));
            CUDA_CHECK(cudaMemcpy(h_vel.data(), d_vel, N * sizeof(float4), cudaMemcpyDeviceToHost));

            printEnergyStats(h_pos, h_vel);
        };
#endif // NBODY_DEBUG


    };
    cudaDeviceSynchronize();

#ifdef NBODY_DEBUG
    std::cout << "simulation ok" << '\n';
#endif

    // frees
    cudaFree(d_pos);
    cudaFree(d_vel);
    cudaFree(d_forces);
    
    return 0;
};
