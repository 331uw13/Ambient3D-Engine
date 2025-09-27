#ifndef AMBIENT3D_TIMER_HPP
#define AMBIENT3D_TIMER_HPP

#include <string_view>
#include <mutex>

namespace AM {

    // Thread safe timer.
    // Measures the time in seconds.
    
    class Timer {
        public:
            Timer(){}
            ~Timer(){}

            // In Milliseconds
            float time_ms() {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_time_sc * 1000.0f;
            }
            
            // In Seconds.
            float time_sc() {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_time_sc;
            }

            void reset() {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_time_sc = 0.0f;
                m_was_reset_this_frame = true;
            }

            void clear_reset_flag() {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_was_reset_this_frame = false;
            }

            bool was_reset_this_frame() { 
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_was_reset_this_frame;
            }

            // The engine calls this function automatically
            // if AM::State::create_named_timer() is used.
            void update(float frame_time) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_time_sc += frame_time;
            }

        private:

            std::mutex m_mutex;
            bool  m_was_reset_this_frame { false };
            float m_time_sc { 0.0f };
    };
};



#endif
