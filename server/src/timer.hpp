#ifndef AMBIENT3D_SERVER_TIMER_HPP
#define AMBIENT3D_SERVER_TIMER_HPP

#include <chrono>


namespace AM {

    class Timer {
        public:

            void start();
            void stop();

            // Time passed between start and stop calls.
            double delta_time_ns(); // in nanoseconds
            double delta_time_ms(); // in milliseconds
            double delta_time_sc(); // in seconds


        private:

            std::chrono::high_resolution_clock::time_point m_start_timepoint;
            double m_delta_time_ns;
    };

};

#endif
