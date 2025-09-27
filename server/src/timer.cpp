#include "timer.hpp"





void AM::Timer::start() {
    m_start_timepoint = std::chrono::high_resolution_clock::now();
    m_delta_time_ns = 0.0;
}

void AM::Timer::stop() {
    const auto end_timepoint = std::chrono::high_resolution_clock::now();
    m_delta_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>
        (end_timepoint - m_start_timepoint).count();
}

double AM::Timer::delta_time_ns() {
    return m_delta_time_ns;
}

double AM::Timer::delta_time_ms() {
    return m_delta_time_ns / 1000000.0;
}

double AM::Timer::delta_time_sc() {
    return m_delta_time_ns / 1000000000.0;
}


