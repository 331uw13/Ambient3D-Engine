#ifndef AMBIENT3D_ASSETS_SERVER_CONFIG_HPP
#define AMBIENT3D_ASSETS_SERVER_CONFIG_HPP

#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace AM {

    struct Config {
        Config(){}
        Config(const char* json_cfg_path);

        int          port;
        std::string  host_dir;
        std::string  allowed_model_file_exts;
        std::string  allowed_texture_file_exts;
    };

};





#endif
