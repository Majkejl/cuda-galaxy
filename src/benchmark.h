#pragma once

#include "simulation.h"

//constexpr int ITERS = 2000;

class Benchmark {
public:
    Benchmark();
    ~Benchmark() = default;

    Benchmark(const Benchmark&) = delete;
    Benchmark& operator=(Benchmark&) = delete;

    void run(int iters);
private:
    Simulation m_sim;
};