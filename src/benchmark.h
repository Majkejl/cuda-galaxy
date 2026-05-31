#pragma once

#include "simulation.h"

constexpr int ITERS = 500;

class Benchmark {
public:
    Benchmark();
    ~Benchmark() = default;

    Benchmark(const Benchmark&) = delete;
    Benchmark& operator=(Benchmark&) = delete;

    void run();
private:
    Simulation m_sim;
};