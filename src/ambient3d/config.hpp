#ifndef AMBIENT3D_CONFIG_DEF_HPP
#define AMBIENT3D_CONFIG_DEF_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

using json = nlohmann::json;


namespace AM {

    struct Config {
        Config(){}
        Config(const char* json_cfg_path);

        std::string fonts_directory;
        std::string font_file;
        int render_distance;
    };

};


#endif
