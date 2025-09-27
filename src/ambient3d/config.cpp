#include <fstream>
#include <cstdio>

#include "config.hpp"


AM::Config::Config(const char* json_cfg_path) {
    std::fstream stream(json_cfg_path);
    if(!stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open server configuration file (%s)\n",
                __func__, json_cfg_path);
        return;
    }

    json data = json::parse(stream);

    this->fonts_directory = data["fonts_directory"].template get<std::string>();
    this->font_file = data["font_file"].template get<std::string>();
    this->render_distance = data["render_distance"].template get<int>();
}


