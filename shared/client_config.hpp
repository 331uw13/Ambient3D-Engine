#ifndef AMBIENT3D_CONFIG_DEF_HPP
#define AMBIENT3D_CONFIG_DEF_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

using json = nlohmann::json;


namespace AM {

    struct ClientConfig {
        ClientConfig(){}
        ClientConfig(const char* json_cfg_path);
        void parse_from_memory(const json& data);

        std::string fonts_directory;
        std::string font_file;
        int render_distance;

        std::string json_data;
    };

};


#endif
