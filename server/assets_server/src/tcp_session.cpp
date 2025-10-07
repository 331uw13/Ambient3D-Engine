#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "tcp_session.hpp"
#include "config.hpp"




void AM::TCP_session::m_handle_received_packet(size_t size) {
    printf("(%li) | %s\n", size, m_data);
}


void AM::TCP_session::m_do_read() {
    
    memset(m_data, 0, Config::MAX_PACKET_SIZE);

    //const std::shared_ptr<TCP_session>& self(shared_from_this());
    m_socket.async_read_some(asio::buffer(m_data, Config::MAX_PACKET_SIZE),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    // TODO: Remove client.
                }
                else {
                    m_handle_received_packet(size);
                }

                m_do_read();
            });
}


/*
void AM::TCP_session::send_packet() {
    if((this->packet.get_flags() & AM::Packet::FLG_WRITE_ERROR)) {
        return;
    }

    auto self(shared_from_this());
    asio::async_write(m_socket, asio::buffer(this->packet.data, this->packet.size),
            [this, self](std::error_code ec, std::size_t) {
                if(ec) {
                    printf("[write_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    m_server->remove_player(self->player_id);
                    return;
                }
            });

    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}
*/

