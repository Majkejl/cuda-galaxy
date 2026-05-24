#pragma once
#include <vector>
#include <cuda_runtime.h>   // float4, cudaGraphicsResource_t, and the host runtime API

// Host-side handle to the GPU simulation. The whole target already links the CUDA
// runtime (CUDA::cudart), so we now expose typed device pointers directly instead of
// hiding them behind void* — simpler and type-safe. The CUDA kernels still live in the
// .cu files; this class only issues host-side runtime/interop calls, so it compiles as
// a plain C++ translation unit (simulation.cpp).
class Simulation {
public:
    Simulation();    // allocate device buffers + build initial conditions
    ~Simulation();   // free device buffers + unregister the interop resource

    Simulation(const Simulation&)            = delete;   // owns raw device memory
    Simulation& operator=(const Simulation&) = delete;

    void step();                          // advance the simulation by one timestep
    void registerVBO(unsigned int vbo);   // register the GL VBO (by name) for CUDA-GL interop

    int particleCount() const;
    const void* initialPositions() const { return m_hInitPos.data(); }
private:
    std::vector<float>     m_hInitPos;             // initial positions, handed to the renderer's VBO
    cudaGraphicsResource_t m_res     = nullptr;    // CUDA handle to the GL VBO (the live positions)
    float4*                m_dVel    = nullptr;    // device velocities
    float4*                m_dForces = nullptr;    // device force/accel scratch
    int                    m_step    = 0;
};
