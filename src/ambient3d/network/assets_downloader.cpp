#include <cstdio>
#include <nlohmann/json.hpp>
#include <filesystem>

#include "assets_downloader.hpp"
#include "shared/include/file_sha256.hpp"
#include "shared/include/packet_parser.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;


void AM::AssetsDownloader::m_handle_recv_data(size_t size) {

    AM::PacketID packet_id = AM::parse_network_packet(m_recv_data, size);

    switch(packet_id) {
        case AM::PacketID::DO_ACCEPT_ASSETS_DOWNLOAD:
            if(size <= sizeof(size_t)) {
                fprintf(stderr, "[AssetsDownloader]: Invalid packet size(%li)"
                        " for DO_ACCEPT_ASSETS_DOWNLOAD\n", size);
                return;
            }
            {
                size_t total_sizeb = 0;
                memmove(&total_sizeb, m_recv_data, sizeof(total_sizeb));

                json files_json = json::parse(m_recv_data + sizeof(size_t));
                printf("------------------------------------------\n");

                // Print size in bytes for each downloadable file.
                for(const json& j : files_json["files"]) {
                    printf(" %-20s - %i Bytes\n",
                            j[0].template get<std::string>().c_str(),
                            j[1].template get<int>());
                }

                //printf("%s\n", m_recv_data + sizeof(size_t));
                printf("\n\033[32mAccept download of %li bytes?\033[0m\n", total_sizeb);
               
                // Wait for user input to get permission to continue downloading.
                // TODO: Maybe gui would be more user friendly 
                // so the game dont need to be launched from command line.
                bool accepted_download = false;
                while(true) {
                    printf(" [Yes/No]: ");
                    fflush(stdout);
                
                    constexpr int INPUTBUF_SIZE = 7;
                    char input_buf[INPUTBUF_SIZE+1] = { 0 };
                    size_t read_size = read(1, input_buf, INPUTBUF_SIZE);
                    
                    if(read_size <= 0) {
                        printf("\n");
                        continue;
                    }

                    if((input_buf[0] == 'Y') || (input_buf[0] == 'y')) {
                        accepted_download = true;
                        break;
                    }
                    if((input_buf[0] == 'N') || (input_buf[0] == 'n')) {
                        m_keep_connection_alive = false;
                        break;
                    }
                }

                if(accepted_download) {
                    m_packet.prepare(AM::PacketID::ACCEPTED_ASSETS_DOWNLOAD);
                    m_send_packet();
                }
            }
            break;

        case AM::PacketID::CREATE_ASSET_FILE:
            try {
                json fileinfo = json::parse(m_recv_data);

                std::string filename  = fileinfo["filename"].template get<std::string>();
                std::string filegroup = fileinfo["filegroup"].template get<std::string>();
                size_t      filesize  = fileinfo["filesize"].template get<size_t>();

                m_download_sizeb = filesize;
                m_download_filename = filename;

                /*
                printf("File name:  \"%s\"\n", filename.c_str());
                printf("File group: \"%s\"\n", filegroup.c_str());
                printf("File size:  %i bytes\n", filesize);
                */

                if(m_download_file.is_open()) {
                    m_download_file.close();
                }

                // TODO: Make filepath "building" more stable.
                std::string filepath = m_config.game_asset_dir + filegroup +"/"+ filename;
                m_download_file.open(filepath, std::ios::binary | std::ios::trunc);

                if(!m_download_file.is_open()) {
                    fprintf(stderr, "[AssetsDownloader(CREATE_ASSET_FILE)]: Failed to create file '%s'\n", filepath.c_str());
                    return;
                }

                /*
                printf("Created file '%s' | Expected file size = %li bytes\n", 
                        filepath.c_str(), filesize);
                */

                printf("\n\n");
                m_packet.prepare(AM::PacketID::CLIENT_CREATED_ASSET_FILE);
                m_send_packet();
            }
            catch(const std::exception& e) {
                fprintf(stderr, "[AssetsDownloader(CREATE_ASSET_FILE)]: %s\n", e.what());
                m_keep_connection_alive = false;
            }
            break;

        case AM::PacketID::ASSET_FILE_BYTES:

            m_downloaded_bytes += size;

            printf("\033[1A %20s - %li / %li bytes\n", 
                    m_download_filename.c_str(),
                    m_downloaded_bytes,
                    m_download_sizeb);


            /*printf("(%i) RECEIVED %li bytes! Total = %li\n", sec++, size, m_downloaded_bytes);

            for(size_t i = 0; i < 16; i++) {
                printf("%02X ", (uint8_t)m_recv_data[i]);
            }
            printf("\n");
            */
            
            m_download_file.write(m_recv_data, size);

            // Inform the server we received some bytes so it will keep sending more.
            m_packet.prepare(AM::PacketID::GOT_SOME_FILE_BYTES);
            m_send_packet();
            break;


        case AM::PacketID::ASSET_FILE_END:
            printf("+ Download complete!\n");
            m_download_file.close();
            break;
    }
}

            
void AM::AssetsDownloader::close_connection(asio::io_context& context) {
    context.stop();
    m_event_handler_th.join();
}


AM::AssetsDownloader::AssetsDownloader(const AM::ClientConfig& config, asio::io_context& context, const char* host, const char* port)
: m_tcp_socket(context), m_config(config) {
    try {

        asio::ip::tcp::resolver tcp_resolver(context);
        asio::connect(m_tcp_socket, tcp_resolver.resolve(host, port));
    
        m_do_read_tcp();

        m_event_handler_th = std::thread([](asio::io_context& context) {
            context.run();
        },
        std::ref(context));

    }
    catch(const std::exception& e) {
        fprintf(stderr, "[AssetsDownloader]: %s\n", e.what());
    }
}

void AM::AssetsDownloader::m_find_local_gamefiles(
        std::vector<mAssetFile>* local_files, 
        const std::string& path,
        const std::string& asset_type
){

    std::string tmp_file_ext = "";
    tmp_file_ext.reserve(8);

    for(const fs::directory_entry& entry : fs::directory_iterator(path + asset_type)) {
        if(entry.is_directory()) {
            continue;
        }

        local_files->push_back(mAssetFile {
                entry.path().filename(),
                asset_type
        });

        AM::compute_sha256_filehash(entry.path(), &local_files->back().sha256_hash);
    }
}

void AM::AssetsDownloader::update_assets() {

    m_packet.allocate_memory();


    json data = json::parse("{}");

    std::vector<mAssetFile> local_texture_files;
    std::vector<mAssetFile> local_model_files;
    std::vector<mAssetFile> local_audio_files;

    if(!fs::exists(m_config.game_asset_dir)) {
        fs::create_directory(m_config.game_asset_dir);
        fs::create_directory(m_config.game_asset_dir + "textures");
        fs::create_directory(m_config.game_asset_dir + "models");
        fs::create_directory(m_config.game_asset_dir + "audio");
    }
    else {
        m_find_local_gamefiles(&local_texture_files, m_config.game_asset_dir, "textures");
        m_find_local_gamefiles(&local_model_files, m_config.game_asset_dir, "models");
        m_find_local_gamefiles(&local_audio_files, m_config.game_asset_dir, "audio");
    }

    printf("[AssetsDownloader]: Found %li local files.\n", 
            local_texture_files.size() + local_model_files.size() + local_audio_files.size());

    for(size_t i = 0; i < local_texture_files.size(); i++) {
        mAssetFile& file = local_texture_files[i];
        data[file.type][i] = { file.name, file.sha256_hash };
    }

    for(size_t i = 0; i < local_model_files.size(); i++) {
        mAssetFile& file = local_model_files[i];
        data[file.type][i] = { file.name, file.sha256_hash };
    }

    for(size_t i = 0; i < local_audio_files.size(); i++) {
        mAssetFile& file = local_audio_files[i];
        data[file.type][i] = { file.name, file.sha256_hash };
    }


    m_packet.prepare(AM::PacketID::CLIENT_GAMEASSET_FILE_HASHES);
    m_packet.write_string({ data.dump() });
    m_send_packet();


    while(m_keep_connection_alive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}



void AM::AssetsDownloader::m_send_packet() {
    if((m_packet.get_flags() & AM::Packet::FLG_WRITE_ERROR)) {
        return;
    }

    asio::async_write(m_tcp_socket, asio::buffer(m_packet.data, m_packet.size),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    fprintf(stderr, "[AssetsDownloader write](%i): %s\n", ec.value(), ec.message().c_str());
                }
            });

    m_packet.enable_flag(AM::Packet::FLG_COMPLETE);
}

void AM::AssetsDownloader::m_do_read_tcp() {

    memset(m_recv_data, 0, AM::MAX_PACKET_SIZE);
    m_tcp_socket.async_read_some(asio::buffer(m_recv_data, AM::MAX_PACKET_SIZE),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    fprintf(stderr, "[AssetsDownloader read](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }

                m_handle_recv_data(size); 
                m_do_read_tcp();
            });

}


