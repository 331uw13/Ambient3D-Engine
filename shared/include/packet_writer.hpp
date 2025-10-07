#ifndef AMBIENT3D_PACKET_WRITER_HPP
#define AMBIENT3D_PACKET_WRITER_HPP

#include <cstddef>
#include <string>
#include <initializer_list>
#include <mutex>
#include <thread>

#include "networking_agreements.hpp"
#include "packet_ids.hpp"



namespace AM {

    class Packet {
        public:

            // This flag is enabled if AM::Packet::write_bytes experiences an error.
            // Then writing is no longer possible for the packet
            // AM::Packet::prepare clears this flag and writing is enabled again.
            static constexpr int FLG_WRITE_ERROR = (1 << 0);
            
            // Functions who automatically mark the packet as complete:
            // AM::Network::send_packet      (client side)
            // AM::TCP_Session::send_packet  (server side)
            // AM::UDP_Handler::send_packet  (server side)
            static constexpr int FLG_COMPLETE = (1 << 1); 

            // Allocates AM::MAX_PACKET_SIZE bytes of memory.
            void allocate_memory();   // < thread safe >
            void free_memory();       // < thread safe >

            char*   data { NULL };
            size_t  size { 0 };

            void enable_flag(int flag);  // < thread safe >
            void disable_flag(int flag); // < thread safe >
            int  get_flags();            // < thread safe >

            // Clears previous data and writes the packet id.
            // Use this function to start writing a new packet.
            //
            // This function will block until another thread finished writing
            // or FLG_COMPLETE is set. 
            // Same thread has ability to re-prepare a packet if needed.
            //
            // This function will bypass FLG_COMPLETE check
            // IF: this->size <= sizeof(AM::PacketID) 
            //     or this->size is equal to zero
            //     or the packet experienced a write error.
            void prepare(AM::PacketID packet_id);        // < thread safe >
            
            bool write_bytes(void* data, size_t sizeb);  // < thread safe >
            bool write_separator();                      // < thread safe >
            bool write_string(std::initializer_list<std::string> list); // < thread safe >
            
            template<typename T>
            bool write(std::initializer_list<T> list) {  // < thread safe >
                for(auto it = list.begin(); it != list.end(); ++it) {
                    if(!write_bytes((void*)it, sizeof(T))) {
                        return false;
                    }
                }
                return true;
            }

        private:
            int m_flags { 0 };
            std::thread::id m_current_thread_id;
            mutable std::mutex m_mutex;
    };
};


#endif
