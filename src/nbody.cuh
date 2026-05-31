#pragma once

#include <cstdio>
#include <cstdlib>
#include <vector>

// Wraps any CUDA runtime call. Prints file, line, and human-readable error string, then exits.
#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t _err = (call);                                              \
        if (_err != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d — %s\n",                      \
                    __FILE__, __LINE__, cudaGetErrorString(_err));              \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

constexpr int   N           = 8192 * 4;
constexpr int   BLOCK_SIZE  = 256;
constexpr float DT          = 0.001f;   // timestep
constexpr float G           = 1.0f;     // simulation units
constexpr float EPS         = 0.1f;     // softening

struct ParticleData {
    float4* pos;
    float4* vel;
};

void stepSimulation(float4* d_pos, float4* d_vel, float4* d_forces);

void printEnergyStats(const std::vector<float4> &h_pos, const std::vector<float4> &h_vel);

