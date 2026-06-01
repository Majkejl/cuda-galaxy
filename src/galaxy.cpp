#include "galaxy.h"
#include <random>
#include <ranges>
#include <numbers>

#include "nbody.cuh"

static inline float3 tilt(const float3 vec, const float angle){
    return float3(vec.x, vec.y * cosf(angle) - vec.z * sinf(angle), vec.y * sinf(angle) + vec.z * cosf(angle));
}

void makeGalaxy(const GalaxyParams &p, std::vector<float4> &pos, std::vector<float4> &vel) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> u(0, 1);
    std::normal_distribution<float> gauss(0.f, 1.f);
    
    float r, k, armOffset, spiral, scatter, theta, dn, v_circ;
    float3 p_local, v_local;

    pos.emplace_back(p.pos.x, p.pos.y, p.pos.z, p.coreMass);
    vel.emplace_back(0.f + p.vel.x, 0.f + p.vel.y, 0.f + p.vel.z, 1.f);
    for (int i = 1; i < p.count; i++){
        
        r = (p.uniform) ? p.radius * sqrtf(u(gen)) :
                         -p.radius * log(u(gen) * u(gen));
        k = floor(u(gen) * p.arms);
        armOffset = k * 2 * std::numbers::pi / p.arms;
        spiral = logf(r/p.radius + 0.0001) / tanf(p.pitch);
        scatter = -log(u(gen) * u(gen)) * p.armSpread;//  * p.radius * p.radius / r;
        theta = armOffset - spiral + scatter;

        auto fr = (p.uniform) ? (r/p.radius) * (r/p.radius) :
                                1 - exp(-r/p.radius) * ( 1 + r/p.radius);
        auto M_disk = (p.count - 1) * p.starMass;
        auto M_enc = p.coreMass + M_disk * fr;

        dn = r * r + EPS * EPS;
        v_circ = sqrtf(G * M_enc * r * r / sqrtf(dn * dn * dn));
        
        p_local = tilt(float3(cosf(theta) * r, sinf(theta) * r, p.thickness * gauss(gen)), p.tiltRadians);
        v_local = tilt(float3(-v_circ * sinf(theta), v_circ * cosf(theta), 0), p.tiltRadians);

        pos.emplace_back(p_local.x + p.pos.x, p_local.y + p.pos.y, p_local.z + p.pos.z, std::max(0.f, p.starMass + p.starMass * gauss(gen) * p.randomness));
        vel.emplace_back(v_local.x + p.vel.x, v_local.y + p.vel.y, v_local.z + p.vel.z, 1.f);
    }
}