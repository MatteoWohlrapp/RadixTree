#pragma once

class Benchmark
{
public:
    template <typename Func>
    void measure(Func func);
};