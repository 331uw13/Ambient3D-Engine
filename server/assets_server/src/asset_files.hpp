#ifndef AMBIENT3D_GAME_ASSETS_SERVER_FILES_HPP
#define AMBIENT3D_GAME_ASSETS_SERVER_FILES_HPP

#include <string>
#include <vector>

#include "config.hpp"


namespace AM {

    struct AssetFile {
        size_t      size;
        std::string full_path;
        std::string name;
        std::string sha256_hash;
        std::string type_group; // "textures" or "models"
    };


    struct AssetFileStorage {
        std::vector<AssetFile> texture_files;
        std::vector<AssetFile> model_files;
        std::vector<AssetFile> audio_files;
    };

    void find_asset_files(const AM::Config& config, AssetFileStorage* file_storage);
    void compute_asset_file_hashes(AssetFileStorage* file_storage);
};




#endif
