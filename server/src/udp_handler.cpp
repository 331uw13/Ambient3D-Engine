#include <cstdio>

#include "server.hpp"
#include "udp_handler.hpp"
#include "packet_ids.hpp"
#include "packet_parser.hpp"


void AM::UDP_handler::m_handle_received_packet(size_t sizeb) {
    AM::PacketID packet_id = AM::parse_network_packet(m_data, sizeb);
    if(packet_id == AM::PacketID::NONE) {
        return;
    }

    if(m_server->show_debug_info) {
        printf("(%i) -> ", packet_id);
        for(size_t i = 0; i < sizeb; i++) {
            printf("0x%x, ", m_data[i]);
        }
        printf("\n");
    }

    switch(packet_id) {

        case AM::PacketID::PLAYER_ID:
            if(sizeb != AM::PacketSize::PLAYER_ID) {
                fprintf(stderr, "ERROR! Unexpected packet size for: "
                        "PLAYER_ID (Got: %li bytes)\n", sizeb);
                return;
            }
            {
                int player_id = -1;
                memmove(&player_id, m_data, sizeof(player_id));

                AM::Player* player = m_server->get_player_by_id(player_id);
                if(!player) {
                    return;
                }
                
                m_server->players_mutex.lock();

                const auto search = m_recv_endpoints.find(player_id);
                if(search == m_recv_endpoints.end()) {
                    m_recv_endpoints.insert(std::make_pair(player_id, m_sender_endpoint));
                }

                //AM::packet_prepare(&player->tcp_session->packet, AM::PacketID::PLAYER_ID_HAS_BEEN_SAVED);
                //AM::packet_write_int(&player->tcp_session->packet, { 1 });
                player->tcp_session->packet.prepare(AM::PacketID::PLAYER_ID_HAS_BEEN_SAVED);
                player->tcp_session->send_packet();
                m_server->players_mutex.unlock();
            }
            break;

        case AM::PacketID::PLAYER_MOVEMENT_AND_CAMERA:
            if(sizeb != AM::PacketSize::PLAYER_MOVEMENT_AND_CAMERA) {
                fprintf(stderr, "ERROR! Unexpected packet size for: "
                        "PLAYER_MOVEMENT_AND_CAMERA (Got: %li bytes)\n",
                        sizeb);
                return;
            }
            {
                int player_id = -1;
                memmove(&player_id, m_data, sizeof(player_id));

                AM::Player* player = m_server->get_player_by_id(player_id);
                if(!player) {
                    return;
                }

                size_t offset = sizeof(player_id);
                
                memmove(&player->anim_id, m_data+offset, sizeof(int));
                offset += sizeof(int);
                
                memmove(&player->pos, m_data+offset, sizeof(float)*3);
                offset += sizeof(float)*3;

                memmove(&player->cam_yaw, m_data+offset, sizeof(float));
                offset += sizeof(float);
                
                memmove(&player->cam_pitch, m_data+offset, sizeof(float));
                //offset += sizeof(float);


            }
            break;
    }
    //printf("(UDP) PacketID: %i\n", packet_id);
}

void AM::UDP_handler::m_do_read() {

    memset(m_data, 0, AM::MAX_PACKET_SIZE);
    m_socket.async_receive_from(
            asio::buffer(m_data, AM::MAX_PACKET_SIZE), m_sender_endpoint,
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read_udp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }

                m_handle_received_packet(size);
                m_do_read();
            });
}


void AM::UDP_handler::send_packet(int player_id) {
    if((this->packet.get_flags() & AM::Packet::FLG_WRITE_ERROR)) {
        return;
    }

    const auto endpoint = m_recv_endpoints.find(player_id);
    if(endpoint == m_recv_endpoints.end()) {
        fprintf(stderr, "ERROR! No UDP endpoint was found for player id (%i)\n",
                player_id);

        // Add player id to queue for trying to send player id to client again
        // so the client will be connected.
        m_server->resend_player_id_queue.push_back(player_id);
        this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
        return;
    } 

    m_socket.async_send_to(
            asio::buffer(this->packet.data, this->packet.size), endpoint->second,
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[write_udp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }
            });

    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}


