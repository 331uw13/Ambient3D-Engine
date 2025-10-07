#include <fstream>
#include <cstdio>

#include "../include/client_config.hpp"


AM::ClientConfig::ClientConfig(const char* json_cfg_path) {
    std::fstream stream(json_cfg_path);
    if(!stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open server configuration file (%s)\n",
                __func__, json_cfg_path);
        return;
    }

    json data = json::parse(stream);
    this->json_data = data.dump();

    printf("%s\n", this->json_data.c_str());

    this->parse_from_memory(data);
}

void AM::ClientConfig::parse_from_memory(const json& data) {
    this->items_directory = data["items_directory"].template get<std::string>();
    this->fonts_directory = data["fonts_directory"].template get<std::string>();
    this->font_file = data["font_file"].template get<std::string>();
    this->render_distance = data["render_distance"].template get<int>();
}


