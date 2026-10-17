#pragma once

#include <glad/glad.h>

#include <array>
#include <string>
#include <vector>

// GPU time between marks, measured with timer queries. The GPU runs a few frames behind the CPU, so the results
// of a frame arrive later: the profiler keeps a ring of query sets and reads a set only when it is ready, so the
// CPU never waits for the GPU.
class GpuProfiler {
public:
    struct Timing {
        std::string name; // the pass that ends at this mark
        float ms = 0.0f;  // averaged over the last frames, so the number is readable
    };

    GpuProfiler();
    ~GpuProfiler();

    // Call beginFrame, then mark(name) at the end of each pass, then endFrame. The names must stay the same
    // each frame.
    void beginFrame();
    void mark(const char* name);
    void endFrame();

    const std::vector<Timing>& timings() const { return results; }
    float totalMs() const;

    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

private:
    static constexpr int kFrames = 3;    // query sets in the ring: enough for the frames the GPU is behind
    static constexpr int kMaxMarks = 8;  // frame start included

    struct Frame {
        std::array<GLuint, kMaxMarks> queries{};
        std::vector<std::string> names; // one for each mark after the start
        bool pending = false;           // issued, the results not read yet
    };

    void read(Frame& frame);

    std::array<Frame, kFrames> frames;
    int current = 0;
    std::vector<Timing> results;
};
