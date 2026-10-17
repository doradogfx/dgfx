#include "renderer/gpu_profiler.h"

#include <algorithm>

GpuProfiler::GpuProfiler() {
    for (Frame& frame : frames)
        glGenQueries(kMaxMarks, frame.queries.data());
}

GpuProfiler::~GpuProfiler() {
    for (Frame& frame : frames)
        glDeleteQueries(kMaxMarks, frame.queries.data());
}

void GpuProfiler::beginFrame() {
    Frame& frame = frames[current];

    // The set is reused. If the GPU has still not finished it, skip its results instead of waiting.
    if (frame.pending) {
        GLint available = 0;
        glGetQueryObjectiv(frame.queries[frame.names.size()], GL_QUERY_RESULT_AVAILABLE, &available);

        if (available)
            read(frame);
    }

    frame.names.clear();
    frame.pending = false;
    glQueryCounter(frame.queries[0], GL_TIMESTAMP); // the GPU writes its clock when it gets here
}

void GpuProfiler::mark(const char* name) {
    Frame& frame = frames[current];

    if (frame.names.size() + 1 >= kMaxMarks)
        return;

    frame.names.push_back(name);
    glQueryCounter(frame.queries[frame.names.size()], GL_TIMESTAMP);
}

void GpuProfiler::endFrame() {
    frames[current].pending = true;
    current = (current + 1) % kFrames;

    // Read the oldest set that is ready. It is the next one to be reused.
    Frame& oldest = frames[current];

    if (!oldest.pending)
        return;

    GLint available = 0;
    glGetQueryObjectiv(oldest.queries[oldest.names.size()], GL_QUERY_RESULT_AVAILABLE, &available);

    if (available) {
        read(oldest);
        oldest.pending = false;
    }
}

void GpuProfiler::read(Frame& frame) {
    // The marks can change between frames (a pass turned off). Start again from the new list then.
    if (results.size() != frame.names.size() ||
        !std::equal(results.begin(), results.end(), frame.names.begin(), [](const Timing& t, const std::string& n) { return t.name == n; })) {
        results.clear();

        for (const std::string& name : frame.names)
            results.push_back({name, 0.0f});
    }

    GLuint64 previous = 0;
    glGetQueryObjectui64v(frame.queries[0], GL_QUERY_RESULT, &previous);

    for (size_t i = 0; i < frame.names.size(); i++) {
        GLuint64 time = 0;
        glGetQueryObjectui64v(frame.queries[i + 1], GL_QUERY_RESULT, &time);

        // Nanoseconds to ms. An exponential average: each new frame moves the number 10% of the way.
        const float ms = static_cast<float>(time - previous) / 1.0e6f;
        results[i].ms = results[i].ms == 0.0f ? ms : results[i].ms * 0.9f + ms * 0.1f;
        previous = time;
    }
}

float GpuProfiler::totalMs() const {
    float total = 0.0f;

    for (const Timing& timing : results)
        total += timing.ms;

    return total;
}
