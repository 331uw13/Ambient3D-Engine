
#include <cstdio>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <lz4.h>

#include <fstream>
#include <iostream>

#include "server.hpp"
#include "math_functions.hpp"


AM::Server::Server(asio::io_context& context, const AM::ServerCFG& cfg) :
    m_tcp_acceptor(context, tcp::endpoint(tcp::v4(), cfg.tcp_port)),
    m_udp_handler(context, cfg.udp_port)
{
    config = cfg;
    if(m_parse_item_list(cfg.item_list_path)) {
        this->start(context);
    }

    m_tcp_acceptor.set_option(asio::socket_base::reuse_address(true)); 
}
        
AM::Server::~Server() {
    m_chunkdata_buf.free_memory();
    printf("Server closed.\n");
}

void AM::Server::m_read_terrain_config() {
    printf("Reading terrain config.\n");
    std::fstream stream(this->config.terrain_config_path);
    if(!stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open server terrain configuration file (%s)\n",
                __func__, this->config.terrain_config_path.c_str());
        return;
    }

    json data = json::parse(stream);
    
    AM::NoiseGenCFG noisecfg {
        .base_frq = data["base_frq"].template get<float>(),
        .base_amp = data["base_amp"].template get<float>(),
        .base_alt = data["base_alt"].template get<float>(),
        .base_detail_frq = data["base_detail_frq"].template get<float>(),
        .base_detail_amp = data["base_detail_amp"].template get<float>(),
        .base_detail_alt = data["base_detail_alt"].template get<float>(),
        .fields_frq = data["fields_frq"].template get<float>(),
        .fields_amp = data["fields_amp"].template get<float>(),
        .mountain_frq = data["mountain_frq"].template get<float>(),
        .mountain_amp = data["mountain_amp"].template get<float>(),
        .mountain_alt = data["mountain_alt"].template get<float>(),
        .mountain_iterations = data["mountain_iterations"].template get<int>(),
        .mountain_iteration_frq_add = data["mountain_iteration_frq_add"].template get<float>(),
        .mountain_iteration_amp_add = data["mountain_iteration_amp_add"].template get<float>(),
        .mountain_iteration_alt_add = data["mountain_iteration_alt_add"].template get<float>(),
    };

    this->terrain.noise_gen = AM::NoiseGen(noisecfg);
}

void AM::Server::start(asio::io_context& io_context) {
    m_chunkdata_buf.allocate(1024 * 1024);
    m_read_terrain_config();

    m_udp_handler.start(this);
    m_do_accept_TCP();

    this->terrain.set_server(this);

    // Set random seed to current time.
    std::srand(std::time({}));

    // Start the event handler.
    std::thread event_handler([](asio::io_context& context) {
        context.run();
    }, std::ref(io_context));

    // Start user input handler.
    m_userinput_handler_th = 
        std::thread(&AM::Server::m_userinput_handler_th__func, this);

    // Start the updater loop.
    m_update_loop_th =
        std::thread(&AM::Server::m_update_loop_th__func, this);
    
    // Start world generator.
    m_worldgen_th =
        std::thread(&AM::Server::m_worldgen_th__func, this);

    // Spawn items for testing
    this->spawn_item(AM::ItemID::M4A16, 1, Vec3{ 3, 3, 16 });
    this->spawn_item(AM::ItemID::HEAVY_AXE, 1, Vec3{ 6, 3, -40 });


    // Input handler may tell to shutdown.
    m_userinput_handler_th.join();
    m_worldgen_th.join();


    io_context.stop();
    event_handler.join();
    m_update_loop_th.join();
    
    this->terrain.delete_terrain();
}

            
void AM::Server::remove_player(int player_id) {
    this->players_mutex.lock();

    const auto search = this->players.find(player_id);
    if(search != this->players.end()) {
        this->players.erase(search);
    }

    this->players_mutex.unlock();

    constexpr size_t msgbuf_size = 512;
    char msgbuf[msgbuf_size] = { 0 };
    snprintf(msgbuf, msgbuf_size-1, "Player %i left the server", player_id);
    this->broadcast_msg(PacketID::SERVER_MESSAGE, msgbuf);
}

void AM::Server::m_do_accept_TCP() {
    
    // Context will call this lambda when connection arrives.
    m_tcp_acceptor.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if(ec) {
                    printf("[accept](%i): %s\n", ec.value(), ec.message().c_str());
                
                    m_do_accept_TCP();
                    return;
                }

                this->players_mutex.lock();

                int player_id = m_next_player_id++; // <- TODO: MAKE SURE THIS WILL NEVER OVERFLOW!
                Player player(std::make_shared<TCP_session>(std::move(socket), this, player_id));
                player.id = player_id;

                this->players.insert(std::make_pair(player_id, player));
                this->players_mutex.unlock();

                player.tcp_session->start();
                
                AM::packet_prepare  (&player.tcp_session->packet, AM::PacketID::PLAYER_ID);
                AM::packet_write_int(&player.tcp_session->packet, { player_id });
                player.tcp_session->send_packet();

                m_do_accept_TCP();
            });

}
            
AM::Player* AM::Server::get_player_by_id(int player_id) {
    this->players_mutex.lock();
    const auto search = this->players.find(player_id);
    if(search == this->players.end()) {
        fprintf(stderr, "ERROR! No player found with ID: %i\n", player_id);
        this->players_mutex.unlock();
        return NULL;
    }

    this->players_mutex.unlock();
    return (AM::Player*)&search->second;
}


void AM::Server::broadcast_msg(AM::PacketID packet_id, const std::string& msg) {
    this->players_mutex.lock();

    for(auto it = this->players.begin(); it != this->players.end(); ++it) {
        Player* p = &it->second;
        AM::packet_prepare(&p->tcp_session->packet, packet_id);
        AM::packet_write_string(&p->tcp_session->packet, msg);
        p->tcp_session->send_packet();
    }

    this->players_mutex.unlock();
}
            
void AM::Server::m_send_player_position(AM::Player* player) {
    if(!player->tcp_session->is_fully_connected()) {
        return;
    }
    
    this->terrain.chunk_map_mutex.lock();
    player->terrain_surface_y = this->terrain.get_surface_level(player->pos);
    this->terrain.chunk_map_mutex.unlock();

    bool on_ground = (player->pos.y <= player->terrain_surface_y);
    AM::ChunkPos player_chunk_pos = this->terrain.get_chunk_pos(player->pos.x, player->pos.z);

    player->pos.y = player->terrain_surface_y + this->config.player_cam_height;

    AM::packet_prepare(&m_udp_handler.packet, AM::PacketID::PLAYER_POSITION);
    AM::packet_write_int(&m_udp_handler.packet, { 
            on_ground,
            player_chunk_pos.x,
            player_chunk_pos.z
    });
    AM::packet_write_int(&m_udp_handler.packet, { player->pos_xz_axis_updated });
    AM::packet_write_float(&m_udp_handler.packet, { player->pos.y });
    if(player->pos_xz_axis_updated) {
        AM::packet_write_float(&m_udp_handler.packet, { player->pos.x, player->pos.z });
        player->pos_xz_axis_updated = false;
    }
    m_udp_handler.send_packet(player->id);
}
            

void AM::Server::m_send_player_updates() {
    this->players_mutex.lock();

    // Tell players each others position, camera yaw and pitch.
    
    // TODO: Wallhacks, "ESP" cheats can be prevented
    // by only telling the player's position when they can see them
    // note to self: think about ways how to bypass this.

    for(auto itA = this->players.begin();
            itA != this->players.end(); ++itA) {
        Player* player = &itA->second;

        player->update_gravity(this->config.gravity);
        m_send_player_position(player);

        for(auto itB = this->players.begin();
                itB != this->players.end(); ++itB) {
            const Player* p = &itB->second;
            if(p->id == player->id) { continue; }

            AM::packet_prepare(&m_udp_handler.packet, AM::PacketID::PLAYER_MOVEMENT_AND_CAMERA);
            AM::packet_write_int(&m_udp_handler.packet, { player->id });
            AM::packet_write_float(&m_udp_handler.packet, {
                        player->pos.x,
                        player->pos.y,
                        player->pos.z,
                        player->cam_yaw,
                        player->cam_pitch
                    });
            AM::packet_write_int(&m_udp_handler.packet, { player->anim_id });

            m_udp_handler.send_packet(p->id);
        }
    }

    this->players_mutex.unlock();
}

void AM::Server::m_send_item_updates() {
    if(this->dropped_items.empty()) {
        return;
    }
    
    this->dropped_items_mutex.lock();

    // Loop through all players online, collect and send nearby item info.

    for(auto it = this->players.begin();
            it != this->players.end(); ++it) {
        const Player* player = &it->second;
        if(!player->tcp_session->is_fully_connected()) {
            continue;
        }

        AM::packet_prepare(&m_udp_handler.packet, AM::PacketID::ITEM_UPDATE);
        uint32_t num_items_nearby = 0;

        for(auto item_it = this->dropped_items.begin(); 
                item_it != this->dropped_items.end(); ++item_it) {
            AM::ItemBase* item = &item_it->second;

            if(player->pos.distance(AM::Vec3(item->pos_x, item->pos_y, item->pos_z))
                    > config.item_near_distance) {
                continue; // Too far away to care.
            }

            AM::packet_write_int(&m_udp_handler.packet, {
                    item->uuid,
                    item->id
            });
            AM::packet_write_float(&m_udp_handler.packet, {
                    item->pos_x,
                    item->pos_y,
                    item->pos_z
            });
            AM::packet_write_string(&m_udp_handler.packet, item->entry_name);
            AM::packet_write_separator(&m_udp_handler.packet);
            
            num_items_nearby++;
        }

        if(num_items_nearby) {
            m_udp_handler.send_packet(player->id);
        }
    }

    this->dropped_items_mutex.unlock();
}

void AM::Server::m_send_player_chunk_updates() {
    this->players_mutex.lock();
    this->terrain.chunk_map_mutex.lock();
            
    const size_t height_points_sizeb = ((this->config.chunk_size+1) * (this->config.chunk_size+1)) * sizeof(float);
    
    std::vector<AM::ChunkPos> chunk_positions;

    for(auto it = this->players.begin();
            it != this->players.end(); ++it) {
        Player* player = &it->second;
        if(!player->tcp_session->is_fully_connected()) {
            continue;
        }



        AM::packet_prepare(&m_udp_handler.packet, AM::PacketID::CHUNK_DATA);
        size_t num_chunks = 0;
       
        chunk_positions.clear();
        m_chunkdata_buf.clear();

        this->terrain.foreach_chunk_nearby(player->pos.x, player->pos.z,
        [this, &num_chunks, &height_points_sizeb, &chunk_positions, player]
        (const AM::Chunk* chunk, const AM::ChunkPos& chunk_pos) {
        
            // Packets may be broken into few little bit smaller packets.
            if(m_chunkdata_buf.size_inbytes() > 1024*32) {
                return; // TODO: return bool to continue loop or break.
            }

            if(!chunk) {
                return;
            }
        
            if(player->loaded_chunks.find(chunk_pos)
                    != player->loaded_chunks.end()) {
                return; // Player has already received this chunk.
            }

            m_chunkdata_buf.write_bytes((void*)&chunk_pos.x, sizeof(chunk_pos.x));
            m_chunkdata_buf.write_bytes((void*)&chunk_pos.z, sizeof(chunk_pos.z));
            m_chunkdata_buf.write_bytes((void*)chunk->height_points, height_points_sizeb);

            player->loaded_chunks.insert(std::make_pair(chunk_pos, true));
            chunk_positions.push_back(chunk_pos);

            num_chunks++;
        });

        if(!num_chunks) {
            continue;
        }

        char* data_source = m_chunkdata_buf.data;
        char* data_destination = &m_udp_handler.packet.data[sizeof(AM::PacketID)];

        ssize_t compressed_size = 
            LZ4_compress_default(
                    data_source,
                    data_destination,
                    m_chunkdata_buf.size_inbytes(),   // Source size.
                    AM::MAX_PACKET_SIZE);             // Destination max size.
        if(compressed_size <= 0) {
            fprintf(stderr, "ERROR! %s: Failed to compress chunk data. (client will not receive an update)\n",
                    __func__);
            
            // Remove "loaded" chunks that were marked as written.
            for(const AM::ChunkPos& chunk_pos : chunk_positions) {
                player->loaded_chunks.erase(chunk_pos);
            }

            continue;
        }

        printf("[CHUNK_UPDATE] (Uncompressed %0.2fkB) -> (Compressed %0.2fkB) to player_id: %i\n",
                (float)m_chunkdata_buf.size_inbytes() / 1000.0f,
                (float)compressed_size / 1000.0f, 
                player->id
                );
        m_udp_handler.packet.status = AM::PacketStatus::HAS_DATA;
        m_udp_handler.packet.size = compressed_size + sizeof(AM::PacketID);
        m_udp_handler.send_packet(player->id);
    }

    this->players_mutex.unlock();
    this->terrain.chunk_map_mutex.unlock();
}

void AM::Server::m_process_resend_id_queue() {
    this->players_mutex.lock();
    for(int player_id : this->resend_player_id_queue) {
        AM::Player* player = this->get_player_by_id(player_id);
        AM::packet_prepare  (&player->tcp_session->packet, AM::PacketID::PLAYER_ID);
        AM::packet_write_int(&player->tcp_session->packet, { player_id });
        player->tcp_session->send_packet();
    }
    this->players_mutex.unlock();

    this->resend_player_id_queue.clear();
}

void AM::Server::m_update_loop_th__func() {
    while(m_keep_threads_alive) {
        m_tick_timer.start();

        m_process_resend_id_queue();
        m_send_player_chunk_updates();
        m_send_player_updates();
        m_send_item_updates();


        m_tick_timer.stop();
        const double delta_time_ms = m_tick_timer.delta_time_ms();
        
        if(delta_time_ms < this->config.tick_delay_ms) {
            // Update finished early, wait some time to try keep correct tick delay.
            std::this_thread::sleep_for(
                    std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::duration<double, std::milli>(this->config.tick_delay_ms - delta_time_ms)));
        }
    }
}

void AM::Server::m_worldgen_th__func() {
    
    while(m_keep_threads_alive) {
        this->players_mutex.lock();
        this->terrain.chunk_map_mutex.lock();

        for(auto it = this->players.begin();
                it != this->players.end(); ++it) {
            const Player* player = &it->second;
            if(!player->tcp_session->is_fully_connected()) {
                continue;
            }

            this->terrain.foreach_chunk_nearby(player->pos.x, player->pos.z,
            [this](const AM::Chunk* chunk, const AM::ChunkPos& chunk_pos) {
                if(!chunk) {
                    AM::Chunk chunk;
                    chunk.generate(this->config, this->terrain.noise_gen, chunk_pos, m_worldgen_seed);
                    this->terrain.add_chunk(chunk);
                    //printf("[WORLD_GEN]: ChunkPos = (%i, %i)\n", chunk_pos.x, chunk_pos.z);
                }
            });
            
        }
        
        this->terrain.chunk_map_mutex.unlock();
        this->players_mutex.unlock();


        std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
    }
}

void AM::Server::m_userinput_handler_th__func() {
    std::string input;

    while(m_keep_threads_alive) {

        std::cin >> input;
        if(input.empty()) { continue; }

        // TODO: Create better system for commands.

        if(input == "end") {
            m_keep_threads_alive = false;
        }
        else
        if(input == "clear") {
            printf("\033[2J\033[H");
            fflush(stdout);
        }
        else
        if(input == "spawn_item") {
            this->spawn_item(AM::ItemID::M4A16, 1, Vec3{ 3, 5, -2 });
        }
        else 
        if(input == "show_debug") {
            this->show_debug_info = true;
        }
        else
        if(input == "hide_debug") {
            this->show_debug_info = false;
        }
        else
        if(input == "online") {
            this->players_mutex.lock();
            printf("Online players: %li\n", this->players.size());
            this->players_mutex.unlock();
        }
        else {
            printf(" Unknown command.\n");
        }

    }

    // NOTE: When the above while loop is exited.
    //       The server will shutdown.
}
            
            
bool AM::Server::m_parse_item_list(const std::string& item_list_path) {
    std::fstream item_list_stream(item_list_path);
    if(!item_list_stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open item list (%s)\n",
                __func__, item_list_path);
        return false;
    }

    item_list = json::parse(item_list_stream);

    this->load_item_template("apple", AM::ItemID::APPLE);
    this->load_item_template("assault_rifle_A", AM::ItemID::M4A16);
    this->load_item_template("heavy_axe", AM::ItemID::HEAVY_AXE);


    return true;
}

void AM::Server::load_item_template(const char* entry_name, AM::ItemID item_id) {
    /*
    std::cout 
        << __func__
        << "(\""
        << entry_name
        << "\") -> " << item_list[entry_name].dump(4) << std::endl;
    */
    this->item_templates[item_id].load_info(item_list, item_id, entry_name);
}


// TODO: Probably good idea to move chunk generation to server side.
void AM::Server::spawn_item(AM::ItemID item_id, int count, const Vec3& pos) {
    if(item_id >= AM::ItemID::NUM_ITEMS) {
        fprintf(stderr, "ERROR! %s: Invalid item_id\n", __func__);
        return;
    }
    
    this->dropped_items_mutex.lock();
    
    int item_uuid = std::rand();
    auto itembase = this->dropped_items.insert({ item_uuid, this->item_templates[item_id] }).first;
    if(itembase == this->dropped_items.end()) {
        fprintf(stderr, "ERROR! %s: Failed to insert item into dropped_items (unordered_map). "
                "UUID may already exist??\n",
                __func__);
        return;
    }
    
    itembase->second.pos_x = pos.x;
    itembase->second.pos_y = pos.y;
    itembase->second.pos_z = pos.z;
    itembase->second.uuid = item_uuid;
    
    printf("%s -> \"%s\" XYZ = (%0.1f, %0.1f, %0.1f) UUID = %i\n", 
            __func__, 
            itembase->second.entry_name,
            pos.x,
            pos.y,
            pos.z,
            item_uuid);
    
    this->dropped_items_mutex.unlock();
}

