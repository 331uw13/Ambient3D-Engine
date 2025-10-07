#include <fstream>

#include "config.hpp"


AM::Config::Config(const char* json_cfg_path) {
    std::fstream stream(json_cfg_path);
    if(!stream.is_open()) {
        fprintf(stderr, "ERROR! %s: Failed to open server configuration file (%s)\n",
                __func__, json_cfg_path);
        return;
    }

    json data = json::parse(stream);
    //this->json_data = data.dump();

    printf("%s\n", data.dump(4).c_str());

    this->port = data["port"].template get<int>();
    this->host_dir = data["host_dir"].template get<std::string>();

    this->allowed_model_file_exts = data["allowed_file_extensions"]["models"].template get<std::string>();
    this->allowed_texture_file_exts = data["allowed_file_extensions"]["textures"].template get<std::string>();

}



