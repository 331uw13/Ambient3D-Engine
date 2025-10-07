#ifndef AMBIENT3D_SERVER_CONFIG_DEF_HPP
#define AMBIENT3D_SERVER_CONFIG_DEF_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

#include "ivec2.hpp"

using json = nlohmann::json;


namespace AM {

    struct ServerCFG {
        ServerCFG(){}
        ServerCFG(const char* json_cfg_path);
        void parse_from_memory(const json& data);

        int tcp_port;
        int udp_port;
        std::string item_list_path;
        std::string terrain_config_path;
        float item_near_distance;
        float item_pickup_distance;
        uint8_t chunk_size;
        int render_distance;
        int chunkdata_uncompressed_max_bytes;
        float chunk_scale;
        float tick_delay_ms;
        float gravity;
        float player_jump_force;
        float player_cam_height;
        float day_cycle_in_minutes;
        AM::iVec2 player_default_inventory_size;

        std::string json_data;

    };

};


#endif
