
#include <vector>
#include <ranges>
#include <cmath>
#include <iostream>
#include "nbody.cuh"

__global__ void computeForces(const float4* pos, float4* forces, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return; // guard for when N isn't a multiple of BLOCK_SIZE

    float fx = 0.f, fy = 0.f, fz = 0.f;
    for (int j = 0; j < n; j++) {
        float dx = pos[j].x - pos[i].x;
        float dy = pos[j].y - pos[i].y;
        float dz = pos[j].z - pos[i].z;
        
        float invDist = rsqrtf(dx * dx + dy*dy + dz*dz + EPS*EPS);
        float s = G * pos[j].w * invDist * invDist * invDist;
        
        fx += dx * s;
        fy += dy * s;
        fz += dz * s;
    };

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

    computeForces<<<numBlocks, BLOCK_SIZE>>>(d_pos, d_forces, N);
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
