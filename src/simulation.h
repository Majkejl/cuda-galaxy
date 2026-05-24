#pragma once

// Host-side handle to the GPU simulation. Deliberately CUDA-free (no float4, no
// cuda_runtime.h) so application.cpp stays a plain C++ translation unit. The device
// pointers are stored as void* here and cast back to float4* inside simulation.cu.
class Simulation {
public:
    Simulation();    // allocate device buffers + upload initial conditions
    ~Simulation();   // free device buffers

    Simulation(const Simulation&)            = delete;   // owns raw device memory
    Simulation& operator=(const Simulation&) = delete;

    void step();     // advance the simulation by one timestep

private:
    void* m_dPos    = nullptr;
    void* m_dVel    = nullptr;
    void* m_dForces = nullptr;
    int   m_step    = 0;
};
