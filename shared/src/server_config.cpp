#include <fstream>
#include <cstdio>

#include "../include/server_config.hpp"


AM::ServerCFG::ServerCFG(const char* json_cfg_path) {
    std::fstream stream(json_cfg_path);
    if(!stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open server configuration file (%s)\n",
                __func__, json_cfg_path);
        return;
    }

    json data = json::parse(stream);
    this->parse_from_memory(data);
}

void AM::ServerCFG::parse_from_memory(const json& data) {
    printf("Reading server config.\n");
    this->tcp_port = data["tcp_port"].template get<int>();
    this->udp_port = data["udp_port"].template get<int>();
    this->item_list_path = data["item_list_path"].template get<std::string>();
    this->item_near_distance = data["item_near_distance"].template get<float>();
    this->chunk_size = data["chunk_size"].template get<uint8_t>();
    this->render_distance = data["render_distance"].template get<int>();
    this->chunkdata_uncompressed_max_bytes = data["chunkdata_uncompressed_max_bytes"].template get<int>();
    this->chunk_scale = data["chunk_scale"].template get<float>();
    this->terrain_config_path = data["terrain_config_path"].template get<std::string>();
    this->tick_delay_ms = data["tick_delay_ms"].template get<float>();
    this->gravity = data["gravity"].template get<float>();
    this->player_jump_force = data["player_jump_force"].template get<float>();
    this->player_cam_height = data["player_camera_height"].template get<float>();
    this->day_cycle_in_minutes = data["day_cycle_in_minutes"].template get<float>();
    this->player_default_inventory_size.x = data["player_default_inventory_size"]["width"].template get<int>();
    this->player_default_inventory_size.y = data["player_default_inventory_size"]["height"].template get<int>();
    this->item_pickup_distance = data["item_pickup_distance"].template get<float>();
    this->json_data = data.dump();
}

