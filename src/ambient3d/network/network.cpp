#include <cstdio>
#include <string>
#include <iostream>

#include "../ambient3d.hpp"
#include "network.hpp"
#include "packet_parser.hpp"
            

void AM::Network::m_attach_main_TCP_packet_callbacks() {
    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::CHAT_MESSAGE,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)sizeb;
        if(!m_fully_connected) {
            return;
        }
        printf("[CHAT]: %s\n", data);
        m_msg_recv_callback(255, 255, 255, data);
    });


    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::SERVER_MESSAGE,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)sizeb;
        if(!m_fully_connected) {
            return;
        }
        printf("[SERVER]: %s\n", data);
        m_msg_recv_callback(255, 200, 50, data);
    });


    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::PLAYER_ID,
    [this](float interval_ms, char* data, size_t sizeb) { 
        if(sizeb != sizeof(this->player_id)) {
            fprintf(stderr, "ERROR! Packet size(%li) doesnt match expected size "
                    "for PLAYER_ID\n", sizeb);
            return;
        }
        memmove(&this->player_id, data, sizeof(this->player_id));

        // Now send the received player id via UDP
        // so the server can save the endpoint.
        //AM::packet_prepare(&this->packet, AM::PacketID::PLAYER_ID);
        //AM::packet_write_int(&this->packet, { this->player_id });
        this->packet.prepare(AM::PacketID::PLAYER_ID);
        this->packet.write<int>({ this->player_id });
        this->send_packet(AM::NetProto::UDP);
        
        // Tell server client connected successfully.
        //AM::packet_prepare(&this->packet, AM::PacketID::PLAYER_CONNECTED);
        //AM::packet_write_int(&this->packet, { this->player_id });
        this->packet.prepare(AM::PacketID::PLAYER_CONNECTED);
        this->packet.write<int>({ this->player_id });
        this->send_packet(AM::NetProto::TCP); 
    });
   

    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::PLAYER_ID_HAS_BEEN_SAVED,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)data; (void)sizeb;
        m_msg_recv_callback(120, 255, 120, "Connected!");
    });
    

    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::SAVE_ITEM_LIST,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)sizeb;
        printf("[NETWORK]: Received server item list.\n");
        printf("%s\n", data);
        m_engine->item_manager.set_item_list(json::parse(data));
        
        this->packet.prepare(AM::PacketID::GET_SERVER_CONFIG);
        this->send_packet(AM::NetProto::TCP);
        //AM::packet_prepare(&this->packet, AM::PacketID::GET_SERVER_CONFIG);
        //this->send_packet(AM::NetProto::TCP);
    });
    

    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::SERVER_CONFIG,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)sizeb;
        printf("[NETWORK]: Received server configuration.\n");
        this->server_cfg.parse_from_memory(json::parse(data));
        m_engine->item_manager.set_server_config(this->server_cfg);

        //AM::packet_prepare(&this->packet, AM::PacketID::PLAYER_FULLY_CONNECTED);
        //this->packet.prepare(AM::PacketID::PLAYER_FULLY_CONNECTED);
        //this->send_packet(AM::NetProto::TCP);
        
        this->packet.prepare(AM::PacketID::CLIENT_CONFIG);
        this->packet.write_string({ m_engine->config.json_data });
        this->send_packet(AM::NetProto::TCP);

        // The regenbuf will not be allocated if it already is.
        m_engine->terrain.allocate_regenbuf(this->server_cfg.chunkdata_uncompressed_max_bytes);
        m_fully_connected = true;
    });

    this->add_packet_callback(
    AM::NetProto::TCP,
    AM::PacketID::SERVER_GOT_CLIENT_CONFIG,
    [this](float interval_ms, char* data, size_t sizeb) {
        this->packet.prepare(AM::PacketID::PLAYER_FULLY_CONNECTED);
        this->send_packet(AM::NetProto::TCP);
    });
}


void AM::Network::m_attach_main_UDP_packet_callbacks() {
    this->add_packet_callback(
    AM::NetProto::UDP,
    AM::PacketID::PLAYER_POSITION,
    [this](float interval_ms, char* data, size_t sizeb) {
        if(!m_fully_connected) {
            return;
        }
        if(sizeb < AM::PacketSize::PLAYER_POSITION_MIN) {
            fprintf(stderr, "ERROR! Packet size(%li) doesnt match expected size "
                    "for PLAYER_POSITION\n", sizeb);
            return;
        }
        
        size_t byte_offset = 0;
        int on_ground = 1;
        int chunk_x = 0;
        int chunk_z = 0;
        int update_axis_flags = 0;
        Vector3 position = { 0, 0, 0 };

        memmove(&on_ground, &data[byte_offset], sizeof(int));
        byte_offset += sizeof(int);

        memmove(&chunk_x, &data[byte_offset], sizeof(int));
        byte_offset += sizeof(int);
        
        memmove(&chunk_z, &data[byte_offset], sizeof(int));
        byte_offset += sizeof(int);

        memmove(&update_axis_flags, &data[byte_offset], sizeof(int));
        byte_offset += sizeof(int);

        m_engine->player.set_chunk_pos(AM::ChunkPos(chunk_x, chunk_z));
        m_engine->player.on_ground = (bool)std::clamp(on_ground, 0, 1);


        if(update_axis_flags != 0) {
            if((update_axis_flags & AM::FLG_PLAYER_UPDATE_XZ_AXIS)
            && (update_axis_flags & AM::FLG_PLAYER_UPDATE_Y_AXIS)) {
                memmove(&position, &data[byte_offset], sizeof(Vector3));
                
                // Update stacks to interpolate the position.
                m_engine->player.Y_pos_update_stack.push_front(position.y);
                m_engine->player.XZ_pos_update_stack.push_front(Vector2(position.x, position.z));
            }
            else
            if((update_axis_flags & AM::FLG_PLAYER_UPDATE_Y_AXIS)
            && !(update_axis_flags & AM::FLG_PLAYER_UPDATE_XZ_AXIS)) { // Only Y
                memmove(&position.y, &data[byte_offset], sizeof(float));
                
                // Update stacks to interpolate the position.
                m_engine->player.Y_pos_update_stack.push_front(position.y);
                m_engine->player.XZ_pos_update_stack.pop_back();
            }
        }
        else {
            m_engine->player.XZ_pos_update_stack.pop_back();
            m_engine->player.Y_pos_update_stack.pop_back();
        }

        /*
        if(update_axis_settings == AM::UPDATE_PLAYER_Y_AXIS) {
            // Packet contains only Y
            memmove(&position.y, &data[byte_offset], sizeof(float));
            m_engine->player.Y_pos_update_stack.push_front(position.y);
        }
        else
        if(update_axis_settings == AM::UPDATE_PLAYER_XYZ_AXIS
        && (sizeb == AM::PacketSize::PLAYER_POSITION_MAX)) {
            // Packet contains X,Y,Z
            memmove(&position, &data[byte_offset], sizeof(Vector3));
            m_engine->player.Y_pos_update_stack.push_front(position.y);
            m_engine->player.XZ_pos_update_stack.push_front(Vector2(position.x, position.z));
        }
        */

        /*
        if(!update_xz_axis && on_ground) {
        }
        else
        if (sizeb == AM::PacketSize::PLAYER_POSITION_MAX) {
        }
        else {
            // Clear previous positions from the stack.
            // They are no longer valid.
        }
        */

        /*
        else {
            fprintf(stderr, "ERROR! Packet size(%li) doesnt match expected size "
                    "for PLAYER_POSITION (update_xz_axis == true)\n", sizeb);
        }
        */

    });



    this->add_packet_callback(
    AM::NetProto::UDP,
    AM::PacketID::CHUNK_DATA,
    [this](float interval_ms, char* data, size_t sizeb) {
        if(!m_fully_connected) {
            return;
        }
        printf("[NETWORK]: Got chunk update of %li bytes\n", sizeb);
        m_engine->terrain.add_chunkdata_to_queue(data, sizeb);
    });


    this->add_packet_callback(
    AM::NetProto::UDP,
    AM::PacketID::ITEM_UPDATE,
    [this](float interval_ms, char* data, size_t sizeb) {
        if(!m_fully_connected) {
           return;
       }
       if(sizeb < (sizeof(int)*2 + sizeof(float)*3)) {
           fprintf(stderr, "ERROR! Packet size(%li) doesnt match expected size "
                   "for ITEM_UPDATE\n", sizeb);
           return;
       }
       
       static AM::ItemBase itembase;

       size_t byte_offset = 0;
       while(byte_offset < sizeb) {

           memmove(&itembase.uuid, &data[byte_offset], sizeof(int));
           byte_offset += sizeof(int);
           
           memmove(&itembase.id, &data[byte_offset], sizeof(int));
           byte_offset += sizeof(int);
           
           memmove(&itembase.pos_x, &data[byte_offset], sizeof(float)*3);
           byte_offset += sizeof(float)*3;

           // Read item entry_name
           memset(itembase.entry_name, 0, AM::ITEM_MAX_ENTRYNAME_SIZE);
           size_t ename_idx = 0;
           while(true) {
               char byte = m_udprecv_data[byte_offset];
               if(byte == AM::PACKET_DATA_SEPARATOR) {
                   byte_offset++; // Increment byte_offset here too 
                                  // or else next item will have
                                  // the separator byte in the name
                   break;
               }

               itembase.entry_name[ename_idx] = byte;

               if(ename_idx++ >= AM::ITEM_MAX_ENTRYNAME_SIZE) {
                   fprintf(stderr, "ERROR! %s: Unexpectedly long entry name (%s)\n",
                           __func__, itembase.entry_name);
                   return;
               }
               if(byte_offset++ >= sizeb) {
                   break;
               }
           }

           // Add the item to queue to be loaded or only updated.
           m_engine->item_manager.add_itembase_to_queue(itembase);
       }
    });


    this->add_packet_callback(
    AM::NetProto::UDP,
    AM::PacketID::PLAYER_MOVEMENT_AND_CAMERA,
    [this](float interval_ms, char* data, size_t sizeb) {
        if(!m_fully_connected) {
            return;
        }
        if(sizeb != AM::PacketSize::PLAYER_MOVEMENT_AND_CAMERA) {
            fprintf(stderr, "ERROR! Packet size(%li) doesnt match expected size "
                    "for PLAYER_MOVEMENT_AND_CAMERA\n", sizeb);
            return;
        }
        
        int player_id = -1;
        memmove(&player_id, m_udprecv_data, sizeof(player_id));

        N_Player player;
        player.id = player_id;

        // Copy the player data if it already exists.
        const auto player_search = this->players.find(player_id);
        if(player_search != this->players.end()) {
            player = player_search->second;
        }

        size_t byte_offset = sizeof(player_id);
        
        memmove(&player.anim_id, &data[byte_offset], sizeof(int));
        byte_offset += sizeof(int);
        
        memmove(&player.pos, &data[byte_offset], sizeof(float)*3);
        byte_offset += sizeof(float)*3;

        memmove(&player.cam_yaw, &data[byte_offset], sizeof(float));
        byte_offset += sizeof(float);
        
        memmove(&player.cam_pitch, &data[byte_offset], sizeof(float));
        //byte_offset += sizeof(float);

        // Insert player data if not in hashmap
        // or replace existing one.
        this->players[player_id] = player;
    });

}


void AM::Network::m_call_packet_callbacks(AM::NetProto protocol, AM::PacketID packet_id, size_t packet_sizeb) {
    std::lock_guard<std::mutex> lock(m_packet_callbacks_mutex);
    
    AM::PacketCallbacks* callbacks = NULL;
    char* data_buffer = NULL;
    if(protocol == AM::NetProto::TCP) {
        callbacks = &m_tcp_packet_callbacks[packet_id];
        data_buffer = m_tcprecv_data;
    }
    else
    if(protocol == AM::NetProto::UDP) {
        callbacks = &m_udp_packet_callbacks[packet_id];
        data_buffer = m_udprecv_data;
    }
    else {
        fprintf(stderr, "ERROR %s: Invalid protocol. PacketID = %i\n", __func__, packet_id);
        return;
    }

    if(callbacks->functions.empty()) {
        fprintf(stderr, "ERROR! %s: PacketID %i doesnt have any callbacks!\n",
                __func__, packet_id);
        return;
    }


    const float packet_interval_ms = this->get_packet_interval_ms(packet_id);
    for(size_t i = 0; i < callbacks->functions.size(); i++) {
        callbacks->functions[i](packet_interval_ms, data_buffer, packet_sizeb);
    }
}

void AM::Network::add_packet_callback
                (AM::NetProto protocol, AM::PacketID packet_id, 
                    std::function<void(float /*interval_ms*/, char* /*data*/, size_t /*sizeb*/)> callback) {
    std::lock_guard<std::mutex> lock(m_packet_callbacks_mutex);

    if(protocol == AM::NetProto::TCP) {
        m_tcp_packet_callbacks[packet_id].functions.push_back(callback);
    }
    else
    if(protocol == AM::NetProto::UDP) {
        m_udp_packet_callbacks[packet_id].functions.push_back(callback);
    }
}
float AM::Network::get_packet_interval_ms(AM::PacketID packet_id) {
    std::lock_guard<std::mutex> lock(m_packet_intervals_mutex);
    return m_packet_intervals[packet_id].latest_interval_ms;
}

void AM::Network::m_update_packet_interval(AM::PacketID packet_id) {
    std::lock_guard<std::mutex> lock(m_packet_intervals_mutex);
    AM::PacketInterval& packet_inter = m_packet_intervals[packet_id];
    const float current_time = GetTime();
    packet_inter.latest_interval_ms = (current_time - packet_inter.begin_time_sc) * 1000.0f;
    packet_inter.begin_time_sc = current_time;
}



AM::Network::Network(asio::io_context& io_context, const NetConnectCFG& cfg)
    : m_tcp_socket(io_context),
      m_udp_socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0))
{
    if(!cfg.msg_recv_callback) {
        fprintf(stderr, "WARNING! %s: No message received callback.\n",__func__);
    }

    try {
        this->packet.allocate_memory();
        m_msg_recv_callback = cfg.msg_recv_callback;

        asio::ip::tcp::resolver tcp_resolver(io_context);
        asio::connect(m_tcp_socket, tcp_resolver.resolve(cfg.host, cfg.tcp_port));

        asio::ip::udp::resolver udp_resolver(io_context);
        m_udp_sender_endpoint = *udp_resolver.resolve(cfg.host, cfg.udp_port).begin();
    
        memset(m_tcprecv_data, 0, AM::MAX_PACKET_SIZE);
        memset(m_udprecv_data, 0, AM::MAX_PACKET_SIZE);

        m_connected = true;
        m_do_read_tcp();
        m_do_read_udp();

        // Start event handler.
        m_event_handler_th = std::thread([](asio::io_context& context){
                context.run();
                }, std::ref(io_context));

        m_attach_main_TCP_packet_callbacks();
        m_attach_main_UDP_packet_callbacks();
    }
    catch(const std::exception& e) {
        fprintf(stderr, "%s: %s\n",
                __func__, e.what());
    }
}

void AM::Network::close(asio::io_context& io_context) {
    printf("Closing network connection...\n");
    io_context.stop();
    m_event_handler_th.join();
    m_connected = false;
    this->packet.free_memory();
}


void AM::Network::send_packet(AM::NetProto proto) {

    // TODO: Check packet status before sending it.
    
    if(proto == AM::NetProto::TCP) {
        m_tcp_data_ready_to_send = true;
        m_do_write_tcp();
    }
    else 
    if(proto == AM::NetProto::UDP) {
        m_udp_data_ready_to_send = true;
        m_do_write_udp();
    }
}

void AM::Network::m_do_read_tcp() {
    memset(m_tcprecv_data, 0, AM::MAX_PACKET_SIZE);
    m_tcp_socket.async_read_some(asio::buffer(m_tcprecv_data, AM::MAX_PACKET_SIZE),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }

                AM::PacketID packet_id = AM::parse_network_packet(m_tcprecv_data, size);
                m_update_packet_interval(packet_id);
                m_call_packet_callbacks(AM::NetProto::TCP, packet_id, size);
                m_do_read_tcp();
            });
}

void AM::Network::m_do_read_udp() {
    memset(m_udprecv_data, 0, AM::MAX_PACKET_SIZE);
    m_udp_socket.async_receive_from(
            asio::buffer(m_udprecv_data, AM::MAX_PACKET_SIZE), m_udp_sender_endpoint,
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read_udp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }

                AM::PacketID packet_id = AM::parse_network_packet(m_udprecv_data, size);
                m_update_packet_interval(packet_id);
                m_call_packet_callbacks(AM::NetProto::UDP, packet_id, size);
                m_do_read_udp(); 
            });
}

void AM::Network::m_do_write_tcp() {
    if(!m_tcp_data_ready_to_send) {
        return;
    }
    m_tcp_data_ready_to_send = false;

    asio::async_write(m_tcp_socket, 
            asio::buffer(this->packet.data, this->packet.size),
            [this](std::error_code ec, std::size_t /*size*/) {
                if(ec) {
                    printf("[write_tcp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }               
            });
    
    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}


void AM::Network::m_do_write_udp() {
    if(!m_udp_data_ready_to_send) {
        return;
    }
    m_udp_data_ready_to_send = false;

    m_udp_socket.async_send_to(
            asio::buffer(this->packet.data, this->packet.size), m_udp_sender_endpoint,
            [this](std::error_code ec, std::size_t) {
                if(ec) {
                    printf("[write_udp](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }
            });
    
    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}

 
void AM::Network::foreach_online_players(std::function<void(AM::N_Player*)> callback) {
    std::lock_guard<std::mutex> lock(this->players_mutex);

    for(auto it = this->players.begin(); it != this->players.end(); ++it) {
        AM::N_Player* player = &it->second;
        callback(player);
    }
}


