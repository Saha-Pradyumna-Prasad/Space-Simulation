#ifndef SPACESIM_CORE_EVENTS_H
#define SPACESIM_CORE_EVENTS_H

#include "core/config.h"

#include <cstdio>
#include <cstring>

namespace spacesim {

enum class FxEventType {
    Impact,
    Fragment,
    Collapse,
    Spawn,
    Shockwave
};

struct FxEvent {
    FxEventType type = FxEventType::Impact;
    double x = 0.0, y = 0.0, z = 0.0;
    double nx = 0.0, ny = 1.0, nz = 0.0;
    double energy = 1.0;
    float color[3] = {1.0f, 0.8f, 0.4f};
};

struct FxQueue {
    FxEvent events[FX_EVENT_QUEUE_SIZE];
    int count = 0;

    void clear() { count = 0; }

    void push(const FxEvent &event) {
        if (count >= FX_EVENT_QUEUE_SIZE) return;
        events[count++] = event;
    }
};

struct SimEvent {
    double time = 0.0;
    char text[160] = {};
    float color[3] = {1.0f, 0.8f, 0.4f};
};

struct EventLog {
    SimEvent entries[EVENT_LOG_SIZE];
    int count = 0;
    int next = 0;

    void clear() {
        count = 0;
        next = 0;
        for (int i = 0; i < EVENT_LOG_SIZE; ++i) entries[i] = SimEvent{};
    }

    void add(double time, const char *text, float r = 1.0f, float g = 0.8f, float b = 0.4f) {
        if (text == nullptr) return;
        SimEvent &event = entries[next];
        event.time = time;
        std::snprintf(event.text, sizeof(event.text), "%s", text);
        event.color[0] = r;
        event.color[1] = g;
        event.color[2] = b;
        next = (next + 1) % EVENT_LOG_SIZE;
        if (count < EVENT_LOG_SIZE) ++count;
    }
};

} // namespace spacesim

#endif
