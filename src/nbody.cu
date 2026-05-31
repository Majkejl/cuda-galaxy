
#include <vector>
#include <ranges>
#include <cmath>
#include <iostream>
#include "nbody.cuh"

template<int N>
__global__ void computeForces(const float4* __restrict__ pos, float4* __restrict__ forces)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    // if (i >= n) return; // guard for when N isn't a multiple of BLOCK_SIZE

    float fx = 0.f, fy = 0.f, fz = 0.f;
    float4 og_pos = pos[i];

    #pragma unroll 1
    for (int block = 0; block < N / BLOCK_SIZE; block++) {
        __shared__ float4 s_pos[BLOCK_SIZE];
        int copy_i = block * BLOCK_SIZE + threadIdx.x;
        s_pos[threadIdx.x] = (copy_i >= N) ? float4(): pos[copy_i];
        __syncthreads();
        
        #pragma unroll
        for (int j = 0; j < BLOCK_SIZE; j++) {
            float dx = s_pos[j].x - og_pos.x;
            float dy = s_pos[j].y - og_pos.y;
            float dz = s_pos[j].z - og_pos.z;
            
            float invDist = rsqrtf(dx * dx + dy*dy + dz*dz + EPS*EPS);
            float s = G * s_pos[j].w * invDist * invDist * invDist;
            
            fx += dx * s;
            fy += dy * s;
            fz += dz * s;
        };
        __syncthreads();
    }

    if (i >= N) return;
    forces[i] = make_float4(fx, fy, fz, 0.f);
}


__global__ void integrate(float4* pos, float4* vel, const float4* forces, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    vel[i].x += forces[i].x * DT;
    vel[i].y += forces[i].y * DT;
    vel[i].z += forces[i].z * DT;

    pos[i].x += vel[i].x * DT;
    pos[i].y += vel[i].y * DT;
    pos[i].z += vel[i].z * DT;
}

void stepSimulation(float4* d_pos, float4* d_vel, float4* d_forces)
{
    int numBlocks = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;

    computeForces<N><<<numBlocks, BLOCK_SIZE>>>(d_pos, d_forces);
    integrate<<<numBlocks, BLOCK_SIZE>>>(d_pos, d_vel, d_forces, N);
}

void printEnergyStats(const std::vector<float4> &h_pos, const std::vector<float4> &h_vel)
{
    float kinetic = 0.f;
    float potential = 0.f;
    float mx = 0.f, my = 0.f, mz = 0.f; // momentum
    for (int i : std::views::iota(0,N)) {
        kinetic += 0.5 * h_pos[i].w * (h_vel[i].x * h_vel[i].x +
                                             h_vel[i].y * h_vel[i].y +
                                             h_vel[i].z * h_vel[i].z);
        
        mx += h_pos[i].w * h_vel[i].x;
        my += h_pos[i].w * h_vel[i].y;
        mz += h_pos[i].w * h_vel[i].z;
        
        for (int j : std::views::iota(i + 1,N)) {
            auto dx = h_pos[j].x - h_pos[i].x; 
            auto dy = h_pos[j].y - h_pos[i].y; 
            auto dz = h_pos[j].z - h_pos[i].z; 
            potential += -G * h_pos[i].w * h_pos[j].w / std::sqrt(
                dx * dx + dy * dy + dz * dz + EPS * EPS
            );
        };
    };
    
    std::cout << "kinetic: " << kinetic << 
                 "; potential: " << potential <<
                 "; k + p: " << kinetic + potential <<
                 "; momentum: (" << mx << ',' << my << ',' << mz << ")\n";
    std::cout << "---\n";
};
