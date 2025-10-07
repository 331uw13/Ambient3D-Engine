
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
    m_udp_handler.packet.free_memory();
    m_chunkdata_buf.free_memory();
    
    for(auto player_it = this->players.begin(); player_it != this->players.end(); ++player_it) {
        AM::Player* player = player_it->second;
        player->free_memory();
    }

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

    m_udp_handler.packet.allocate_memory();
    m_udp_handler.start(this);
    m_do_accept_TCP();

    this->fog.density = 0.005f;
    this->fog.set_color(0.5f, 0.5f, 0.5f);

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
    this->spawn_item(AM::ItemID::M4A16, 1, Vec3{ 3, 2, 16 });
    this->spawn_item(AM::ItemID::HEAVY_AXE, 1, Vec3{ 6, 2, -40 });


    // Input handler may tell to shutdown.
    m_userinput_handler_th.join();
    m_worldgen_th.join();


    io_context.stop();
    event_handler.join();
    m_update_loop_th.join();
    
    this->terrain.delete_terrain();
}

            
void AM::Server::remove_player(int player_id) {
    const auto search = this->players.find(player_id);
    if(search == this->players.end()) {
        fprintf(stderr, "ERROR! %s: Trying to remove player id (%i). But it doesnt exist.\n",
                __func__, player_id);
        return;
    }

    AM::Player* player = search->second;
    player->free_memory();

    this->players.erase(search);
    delete player;

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

                int player_id = m_next_player_id++; // <- TODO: MAKE SURE THIS WILL NEVER OVERFLOW!
                
                AM::Player* player = new AM::Player(std::make_shared<AM::TCP_session>(std::move(socket), this, player_id));
                player->set_id(player_id);
                player->set_server(this);
                player->inventory.create(AM::InventorySize {
                        .num_slots_x = (uint8_t)this->config.player_default_inventory_size.x,
                        .num_slots_y = (uint8_t)this->config.player_default_inventory_size.y,
                });

                this->players.insert(std::make_pair(player_id, player)).first;

                player->tcp_session->packet.allocate_memory();
                player->tcp_session->start();
                
                player->tcp_session->packet.prepare(AM::PacketID::PLAYER_ID);
                player->tcp_session->packet.write<int>({ player_id });
                player->tcp_session->send_packet();

                printf("[SERVER]: Player(%i) has been prepared.\n", player_id);

                m_do_accept_TCP();
            });
}

AM::Player* AM::Server::get_player_by_id(int player_id) {
    const auto search = this->players.find(player_id);
    if(search == this->players.end()) {
        fprintf(stderr, "ERROR! No player found with ID: %i\n", player_id);
        return NULL;
    }

    return search->second;
}


void AM::Server::broadcast_msg(AM::PacketID packet_id, const std::string& msg) {

    for(auto it = this->players.begin(); it != this->players.end(); ++it) {
        Player* p = it->second;
        p->tcp_session->packet.prepare(packet_id);
        p->tcp_session->packet.write_string({ msg });
        p->tcp_session->send_packet();
    }
}
            
void AM::Server::m_send_player_position(AM::Player* player) {
    if(!player->tcp_session->is_fully_connected()) {
        return;
    }
    
    
    AM::Vec3 player_pos = player->next_position();
    AM::ChunkPos player_chunk_pos = player->chunk_pos();
    int update_axis_flags = player->next_position_flags();

    m_udp_handler.packet.prepare(AM::PacketID::PLAYER_POSITION);

    m_udp_handler.packet.write<int>({ 
        player->on_ground,
        player_chunk_pos.x,
        player_chunk_pos.z,
        update_axis_flags
    });

    if(update_axis_flags != 0) {
        if((update_axis_flags & AM::FLG_PLAYER_UPDATE_XZ_AXIS)
        && (update_axis_flags & AM::FLG_PLAYER_UPDATE_Y_AXIS)) {
            m_udp_handler.packet.write<float>({
                    player_pos.x,
                    player_pos.y,
                    player_pos.z
            });
        }
        else
        if((update_axis_flags & AM::FLG_PLAYER_UPDATE_Y_AXIS)
        && !(update_axis_flags & AM::FLG_PLAYER_UPDATE_XZ_AXIS)) { // Only Y
            m_udp_handler.packet.write<float>({ player_pos.y });
        }
    }

    player->clear_next_position_flags();
    m_udp_handler.send_packet(player->id());
}
                 
void AM::Server::m_send_player_weather_data(AM::Player* player) {
    m_udp_handler.packet.prepare(AM::PacketID::WEATHER_DATA);
    m_udp_handler.packet.write<float>({
            fog.density,
            fog.color_r,
            fog.color_g,
            fog.color_b
    });
    m_udp_handler.send_packet(player->id());
}

void AM::Server::m_send_player_updates() {

    // Tell players each others position, camera yaw and pitch.
    
    // TODO: Wallhacks, "ESP" cheats can be prevented
    // by only telling the player's position when they can see them
    // note to self: think about ways how to bypass this.

    for(auto itA = this->players.begin();
            itA != this->players.end(); ++itA) {
        Player* player = itA->second;
        if(!player->tcp_session->is_fully_connected()) {
            continue;
        }

        player->update();
        m_send_player_position(player);
        m_send_player_weather_data(player);

        for(auto itB = this->players.begin();
                itB != this->players.end(); ++itB) {
            const Player* p = itB->second;
            if(!p->tcp_session->is_fully_connected()) {
                continue;
            }
            if(p->id() == player->id()) {
                continue;
            }

            AM::Vec3 player_pos = player->position();

            m_udp_handler.packet.prepare(AM::PacketID::PLAYER_MOVEMENT_AND_CAMERA);
            m_udp_handler.packet.write<int>({ player->id(), player->animation_id() });
            m_udp_handler.packet.write<float>({
                    player_pos.x,
                    player_pos.y,
                    player_pos.z,
                    player->cam_yaw(),
                    player->cam_pitch()
            });

            m_udp_handler.send_packet(p->id());
        }
    }
}

void AM::Server::m_send_item_updates() {
    if(this->dropped_items.empty()) {
        return;
    }
    
    this->dropped_items_mutex.lock();

    // Loop through all players online, collect and send nearby item info.

    for(auto it = this->players.begin();
            it != this->players.end(); ++it) {
        const Player* player = it->second;
        if(!player->tcp_session->is_fully_connected()) {
            continue;
        }

        m_udp_handler.packet.prepare(AM::PacketID::ITEM_UPDATE);
        uint32_t num_items_nearby = 0;

        for(auto item_it = this->dropped_items.begin(); 
                item_it != this->dropped_items.end(); ++item_it) {
            AM::ItemBase* item = &item_it->second;

            if(player->position().distance(AM::Vec3(item->pos_x, item->pos_y, item->pos_z))
                    > config.item_near_distance) {
                continue; // Too far away to care.
            }

            m_udp_handler.packet.write<int>({
                    item->uuid,
                    item->id
            });
            m_udp_handler.packet.write<float>({
                    item->pos_x,
                    item->pos_y,
                    item->pos_z
            });

            m_udp_handler.packet.write_string({ item->entry_name });
            m_udp_handler.packet.write_separator();
            
            num_items_nearby++;
        }

        if(num_items_nearby) {
            m_udp_handler.send_packet(player->id());
        }
    }

    this->dropped_items_mutex.unlock();
}

void AM::Server::m_send_player_chunk_updates() {
    this->terrain.chunk_map_mutex.lock();
            
    const size_t height_points_sizeb = ((this->config.chunk_size+1) * (this->config.chunk_size+1)) * sizeof(float);
    
    std::vector<AM::ChunkPos> chunk_positions;

    for(auto it = this->players.begin();
            it != this->players.end(); ++it) {
        Player* player = it->second;
        if(!player->tcp_session->is_fully_connected()) {
            continue;
        }


        m_udp_handler.packet.prepare(AM::PacketID::CHUNK_DATA);
        size_t num_chunks = 0;
       
        chunk_positions.clear();
        m_chunkdata_buf.clear();

        AM::Vec3 player_pos = player->position();

        this->terrain.foreach_chunk_nearby(player_pos.x, player_pos.z,
        player->tcp_session->config.render_distance,
        [this, &num_chunks, &height_points_sizeb, &chunk_positions, player]
        (const AM::Chunk* chunk, const AM::ChunkPos& chunk_pos) {
        
            // Packets may be broken into few little bit smaller packets.
            if(m_chunkdata_buf.size_inbytes() >= AM::MAX_PACKET_SIZE) {
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

            m_udp_handler.packet.enable_flag(AM::Packet::FLG_COMPLETE);
            continue;
        }

        printf("[CHUNK_UPDATE] (Uncompressed %0.2fkB) -> (Compressed %0.2fkB) to player_id: %i\n",
                (float)m_chunkdata_buf.size_inbytes() / 1000.0f,
                (float)compressed_size / 1000.0f, 
                player->id());

        m_udp_handler.packet.size = compressed_size + sizeof(AM::PacketID);
        m_udp_handler.send_packet(player->id());
    }

    this->terrain.chunk_map_mutex.unlock();
}

void AM::Server::m_process_resend_id_queue() {
    for(int player_id : this->resend_player_id_queue) {
        AM::Player* player = this->get_player_by_id(player_id);
        player->tcp_session->packet.prepare(AM::PacketID::PLAYER_ID);
        player->tcp_session->packet.write<int>({ player_id });
        player->tcp_session->send_packet();
    }

    this->resend_player_id_queue.clear();
}


void AM::Server::m_update_timeofday(float update_interval_ms) {
    this->timeofday += (update_interval_ms/1000.0f) / (this->config.day_cycle_in_minutes * 60.0f);
    if(this->timeofday >= 1.0f) {
        this->timeofday = 0.0f;
    }
}



void AM::Server::m_update_loop_th__func() {
    while(m_keep_threads_alive) {
        m_update_timer.start();
        m_tick_timer.start();

        m_process_resend_id_queue();
        m_send_player_chunk_updates();
        m_send_player_updates();
        m_send_item_updates();
        m_send_player_itemuuid_unloads();

        m_update_timer.stop();
        const double update_delta_time_ms = m_update_timer.delta_time_ms();

        
        if(update_delta_time_ms < this->config.tick_delay_ms) {
            // Update finished early, wait some time to try keep correct tick delay.
            std::this_thread::sleep_for(
                    std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::duration<double, std::milli>(this->config.tick_delay_ms - update_delta_time_ms)));
        }

        m_tick_timer.stop();
        m_update_timeofday(m_tick_timer.delta_time_ms());
        //m_update_timeofday(delta_time_ms + (this->config.tick_delay_ms - delta_time_ms));
    }
}

void AM::Server::m_worldgen_th__func() {
    
    while(m_keep_threads_alive) {
        this->terrain.chunk_map_mutex.lock();

        for(auto it = this->players.begin();
                it != this->players.end(); ++it) {
            const Player* player = it->second;
            if(!player->tcp_session->is_fully_connected()) {
                continue;
            }

            AM::Vec3 player_pos = player->position();

            this->terrain.foreach_chunk_nearby(player_pos.x, player_pos.z,
            player->tcp_session->config.render_distance,
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
            printf("Online players: %li\n", this->players.size());
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

            
            
void AM::Server::unload_dropped_item(int item_uuid) {
    std::lock_guard<std::mutex> lock(m_player_itemuuid_unload_queue_mutex);   
    m_player_itemuuid_unload_queue.push_back(item_uuid);
}


void AM::Server::m_send_player_itemuuid_unloads() {
    std::lock_guard<std::mutex> lock1(m_player_itemuuid_unload_queue_mutex);
    std::lock_guard<std::mutex> lock2(this->dropped_items_mutex);

    for(size_t item_i = 0; item_i < m_player_itemuuid_unload_queue.size(); item_i++) {
        int item_uuid = m_player_itemuuid_unload_queue[item_i];

        auto item_search = this->dropped_items.find(item_uuid);
        if(item_search == this->dropped_items.end()) {
            continue;
        }

        AM::ItemBase& itembase = item_search->second;
        AM::Vec3 item_pos = AM::Vec3(itembase.pos_x, itembase.pos_y, itembase.pos_z);

        for(auto player_it = this->players.begin(); 
                player_it != this->players.end(); ++player_it)  {

            AM::Player* player = player_it->second;

            if(item_pos.distance(player->position()) > this->config.item_near_distance) {
                continue; // Too far away, player doesnt have this item unloaded.
            }

            player->tcp_session->packet.prepare(AM::PacketID::PLAYER_UNLOAD_DROPPED_ITEM);
            player->tcp_session->packet.write<int>({ itembase.uuid });
            player->tcp_session->send_packet();
        }

        this->dropped_items.erase(item_search);
    }

    m_player_itemuuid_unload_queue.clear();
}
