#pragma once

#include <vector>
#include <vector_types.h>

struct GalaxyParams{
    unsigned count;
    float3 vel = {0.f, 0.f, 0.f};
    float3 pos = {0.f, 0.f, 0.f};
    float radius = 1.f;
    float thickness = 0.02f;
    float coreMass = 2.f;
    float starMass = 0.00005f;
    float tiltRadians = 0.f;
    float randomness = 0.2f;
};

// Appends p.count particles. For each: pos = (x,y,z, mass), vel = (vx,vy,vz, 0).
// The FIRST appended particle is the central core.
void makeGalaxy(const GalaxyParams &p, std::vector<float4> &pos, std::vector<float4> &vel);