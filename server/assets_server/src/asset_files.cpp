
#include <filesystem>

namespace fs = std::filesystem;


#include "asset_files.hpp"
#include "shared/include/file_sha256.hpp"



static void scan_directories(
        const AM::Config& config,
        const std::string& path,
        AM::AssetFileStorage* file_storage) {

    std::string tmp_file_ext = "";
    tmp_file_ext.reserve(8);

    for(const fs::directory_entry& entry : fs::directory_iterator(path)) {
        const char* filename = entry.path().filename().c_str();
    
        if(entry.is_directory()) {
            scan_directories(config, entry.path(), file_storage);
            continue;
        }
            
        printf("- %-16li %-16s ", entry.file_size(), filename);
        fflush(stdout);

        // TODO: It would be better to scan the file metadata for the type.
        if(!entry.path().has_extension()) {
            printf(" (WARNING: No file extension!)\n");
            continue;
        }

        tmp_file_ext.assign(entry.path().extension());
        if(config.allowed_texture_file_exts.find(tmp_file_ext) != std::string::npos) {
            printf(" (Texture)\n");
            file_storage->texture_files.push_back({ 
                    .size = entry.file_size(),
                    .full_path = entry.path(),
                    .name = filename,
            });
        }
        else
        if(config.allowed_model_file_exts.find(tmp_file_ext) != std::string::npos) {
            printf(" (3D Model)\n");
            file_storage->model_files.push_back({ 
                            .size = entry.file_size(),
                    .full_path = entry.path(),
                    .name = filename,
            });
        }
        else {
            printf(" (Ignored)\n");
        }
    }
}


void AM::find_asset_files(const AM::Config& config, AM::AssetFileStorage* file_storage) {

    scan_directories(config, config.host_dir, file_storage);

    printf("-------------------------------------\n");
    printf("Found %li Texture files\n", file_storage->texture_files.size());
    printf("Found %li Model files\n", file_storage->model_files.size());
    printf("Found %li Audio files\n", file_storage->audio_files.size());

}


void AM::compute_asset_file_hashes(AssetFileStorage* file_storage) {
    for(AM::AssetFile& file : file_storage->texture_files) {
        AM::compute_sha256_filehash(file.full_path, &file.sha256_hash);
    }
    for(AM::AssetFile& file : file_storage->model_files) {
        AM::compute_sha256_filehash(file.full_path, &file.sha256_hash);
    }
    for(AM::AssetFile& file : file_storage->audio_files) {
        AM::compute_sha256_filehash(file.full_path, &file.sha256_hash);
    }
}




