#ifndef AMBIENT3D_THREAD_SAFE_STACK_HPP
#define AMBIENT3D_THREAD_SAFE_STACK_HPP

#include <sys/types.h>
#include <mutex>
#include <deque>


namespace AM {

    template<typename T>
    class Stackts {
        public:

            // If size limit is set to positive value
            // Element at back of the stack are going to be removed
            // if the stack element count would grow over m_size_limit.
            // It can also be removed by passing a negative value to this function.
            // By default it doesnt have size limit.
            void set_size_limit(ssize_t limit) { m_size_limit = limit; }


            void push_front(const T& value) {
                m_mutex.lock();
                m_check_size_limit();
                m_stack.push_front(value);
                m_mutex.unlock();
            }

            void push_back(const T& value) {
                m_mutex.lock();
                m_check_size_limit();
                m_stack.push_back(value);
                m_mutex.unlock();
            }

            void pop_front() {
                m_mutex.lock();
                if(m_stack.empty()) {
                    m_mutex.unlock();
                    return;
                }
                m_stack.pop_front();
                m_mutex.unlock();
            }

            void pop_back() {
                m_mutex.lock();
                if(m_stack.empty()) {
                    m_mutex.unlock();
                    return;
                }
                m_stack.pop_back();
                m_mutex.unlock();
            }

            void clear() {
                m_mutex.lock();
                m_stack.clear();
                m_mutex.unlock();
            }

            [[nodiscard]] size_t size() const {
                m_mutex.lock();
                size_t _size = m_stack.size();
                m_mutex.unlock();
                return _size;
            }

            [[nodiscard]] bool empty() const {
                m_mutex.lock();
                bool is_empty = m_stack.empty();
                m_mutex.unlock();
                return is_empty;
            }

            [[nodiscard]] T read_index(size_t index) const {
                m_mutex.lock();
                if(index >= m_stack.size()) {
                    m_mutex.unlock();
                    return T();
                }
                T value = m_stack[index];
                m_mutex.unlock();
                return value;
            }

            [[nodiscard]] T read_back() const {
                m_mutex.lock();
                if(m_stack.empty()) {
                    m_mutex.unlock();
                    return T();
                }
                T value = m_stack.back();
                m_mutex.unlock();
                return value;
            }

        private:

            void m_check_size_limit() {
                if(m_size_limit < 0) {
                    return;
                }
                if(m_stack.size()+1 > (size_t)m_size_limit) {
                    m_stack.pop_back();
                }
            }

            ssize_t                  m_size_limit { -1 };
            mutable std::mutex       m_mutex;
            std::deque<T>            m_stack;
    };
};



#endif
