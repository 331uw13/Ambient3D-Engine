#include <cstdio>
#include <cstring>
#include <chrono>
#include <iostream>

#include "packet_writer.hpp"


void AM::Packet::allocate_memory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_flags |= FLG_COMPLETE;
    
    if(this->data) {
        fprintf(stderr, "ERROR! Trying to allocate memory for packet data. "
                "But it already has address of: %p\n", this->data);
        return;
    }
    this->data = new char[AM::MAX_PACKET_SIZE];
    if(!this->data) {
        fprintf(stderr, "ERROR! Failed to allocate %li bytes of memory for packet data array.\n",
                AM::MAX_PACKET_SIZE);
        return;
    }
}

void AM::Packet::free_memory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if(this->data) {
        delete[] this->data;
        this->data = NULL;
    }
}

void AM::Packet::enable_flag(int flag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_flags |= flag;
}

void AM::Packet::disable_flag(int flag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_flags &= ~flag;
}
            
int AM::Packet::get_flags() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_flags;
}

// Clears previous data and writes the packet id.
// And m_has_write_error is set to false.
void AM::Packet::prepare(AM::PacketID packet_id) {
    //std::lock_guard<std::mutex> lock(m_mutex);
    m_mutex.lock();

    const std::thread::id this_thread_id = std::this_thread::get_id();

    if(!(m_flags & FLG_COMPLETE) 
    && !(m_flags & FLG_WRITE_ERROR)
    && ((this->size > sizeof(AM::PacketID)) || (this->size == 0))
    && (m_current_thread_id != this_thread_id))
    {
        printf("%s: FUCK\n",__func__);
        m_mutex.unlock();
        printf("WARNING! %s() at \"%s\": Waiting until another thread finishes writing...\n",
                __func__, __FILE__);
        
        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            m_mutex.lock();
            if(!(m_flags & FLG_COMPLETE)) {
                m_mutex.unlock();
                continue;
            }
            break;
        }
    }

    if(packet_id >= AM::PacketID::NUM_PACKETS) {
        fprintf(stderr, "ERROR! %s: Cant prepare packet with invalid packet id (%i)\n",
                __func__, packet_id);
        m_mutex.unlock();
        return;
    }

    m_flags = 0;
    m_current_thread_id = this_thread_id;
    memset(this->data, 0, (this->size < AM::MAX_PACKET_SIZE) ? this->size : AM::MAX_PACKET_SIZE);
    this->size = 0;

    memmove(this->data, &packet_id, sizeof(AM::PacketID));
    this->size += sizeof(AM::PacketID);
        
    m_mutex.unlock();
}
            

bool AM::Packet::write_bytes(void* data, size_t sizeb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if((m_flags & FLG_WRITE_ERROR)) {
        fprintf(stderr, "ERROR! %s: Cant write to current packet anymore."
                " It seems it experienced a write error. Maybe too much data?\n",
                __func__);
        return false;
    }

    if((this->size + sizeb) >= AM::MAX_PACKET_SIZE) {
        fprintf(stderr, "ERROR! %s: Packet cant hold more data than %li bytes.\n",
                __func__, AM::MAX_PACKET_SIZE);
        m_flags |= AM::Packet::FLG_WRITE_ERROR;
        return false;
    }

    memmove(this->data + this->size, data, sizeb);
    this->size += sizeb;
    return true;
}

bool AM::Packet::write_separator() {
    return this->write_bytes((void*)&AM::PACKET_DATA_SEPARATOR, sizeof(AM::PACKET_DATA_SEPARATOR));
}
            
bool AM::Packet::write_string(std::initializer_list<std::string> list) {
    for(auto it = list.begin(); it != list.end(); ++it) {
        if(!write_bytes((void*)it->data(), it->size())) {
            return false;
        }
    }
    return true;
}

