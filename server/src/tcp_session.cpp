#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "tcp_session.hpp"
#include "server.hpp"

// From '../../shared'
#include "packet_ids.hpp"
#include "packet_parser.hpp"


AM::TCP_session::TCP_session(tcp::socket socket, AM::Server* server, int player_id) 
    : m_socket(std::move(socket)), m_server(server)
{
    this->player_id = player_id;
    /*
    // Generate UUID for the client.
    printf("UUID: ");
    std::srand(std::time({}));
    for(size_t i = 0; i < AM::UUID_LENGTH-1; i++) {
        this->uuid[i] = abs((char)(std::rand() % 255));
        printf("%x", this->uuid[i]);
    }
    printf("\n");
    this->uuid[AM::UUID_LENGTH-1] = 0;
    */
}
            

void AM::TCP_session::m_handle_received_packet(size_t sizeb) {
    AM::PacketID packet_id = AM::parse_network_packet(m_data, sizeb);
    if(packet_id == AM::PacketID::NONE) {
        return;
    }

    switch(packet_id) {
        case AM::PacketID::CHAT_MESSAGE:
            {
                if(sizeb == 0) { return; }
                if(sizeb > 512) {
                    fprintf(stderr, "[CHAT_WARNING]: Ignored %li long message.\n", sizeb);
                    return;
                }

                // Allow only printable ascii characters.
                // TODO: Good idea is to add support for different languages.
                for(size_t i = 0; i < sizeb; i++) {
                    if((m_data[i] < 0x20) || (m_data[i] > 0x7E)) {
                        return;
                    }
                }

                printf("[CHAT(%li)]: %s\n", sizeb, m_data);
                m_server->broadcast_msg(AM::PacketID::CHAT_MESSAGE, m_data);
            }
            break;

        case AM::PacketID::PLAYER_CONNECTED:
            {    
                // Respond by sending item list.
                this->packet.prepare(AM::PacketID::SAVE_ITEM_LIST);
                this->packet.write_string({ m_server->item_list.dump() });
                this->send_packet();    
            }
            break;

        case AM::PacketID::GET_SERVER_CONFIG:
            {
                // ***********************************************
                // TODO: Remove filepaths before sending to client.
                // ***********************************************
                this->packet.prepare(AM::PacketID::SERVER_CONFIG);
                this->packet.write_string({ m_server->config.json_data });
                this->send_packet(); 
            }
            break;

        case AM::PacketID::PLAYER_FULLY_CONNECTED:
            {
                m_fully_connected = true;
                printf("Player connected. (ID: %i)\n", this->player_id);
            }
            break;

        case AM::PacketID::PLAYER_UNLOADED_CHUNKS:
            if(!m_fully_connected) {
                return;
            }
            if(sizeb < AM::PacketSize::PLAYER_UNLOADED_CHUNKS_MIN) {
                fprintf(stderr, "ERROR! Unexpected packet size for: "
                        "PLAYER_UNLOADED_CHUNKS (Got: %li bytes)\n",
                        sizeb);
                return;
            }
            {
                size_t byte_offset = 0;

                int chunk_x = 0;
                int chunk_z = 0;
                    
                AM::Player* player = m_server->get_player_by_id(this->player_id);
                if(!player) {
                    fprintf(stderr, "ERROR! %s: Cant find player with ID = %i\n", this->player_id);
                    return;
                }

                while(byte_offset < sizeb) {
                    memmove(&chunk_x, &m_data[byte_offset], sizeof(int));
                    byte_offset += sizeof(int);

                    memmove(&chunk_z, &m_data[byte_offset], sizeof(int));
                    byte_offset += sizeof(int);

                    auto chunk_search = player->loaded_chunks.find(AM::ChunkPos(chunk_x, chunk_z));
                    if(chunk_search == player->loaded_chunks.end()) {
                        continue;
                    }

                    player->loaded_chunks.erase(chunk_search);
                }
            }
            break;

        // ...
    }
}

void AM::TCP_session::m_do_read() {
    
    memset(m_data, 0, AM::MAX_PACKET_SIZE);

    const std::shared_ptr<TCP_session>& self(shared_from_this());
    m_socket.async_read_some(asio::buffer(m_data, AM::MAX_PACKET_SIZE),
            [this, self](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    m_server->remove_player(self->player_id);
                    return;
                }
                
                m_handle_received_packet(size);
                m_do_read();
            });

}


void AM::TCP_session::send_packet() {
    if((this->packet.get_flags() & AM::Packet::FLG_WRITE_ERROR)) {
        return;
    }

    auto self(shared_from_this());
    asio::async_write(m_socket, asio::buffer(this->packet.data, this->packet.size),
            [this, self](std::error_code ec, std::size_t /*size*/) {
                if(ec) {
                    printf("[write_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    m_server->remove_player(self->player_id);
                    return;
                }
            });
    
    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}


