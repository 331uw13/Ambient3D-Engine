#ifndef AMBIENT3D_GAME_ASSETS_DOWNLOADER_HPP
#define AMBIENT3D_GAME_ASSETS_DOWNLOADER_HPP

#include <asio.hpp>
#include <fstream>
#include "shared/include/networking_agreements.hpp"
#include "shared/include/client_config.hpp"
#include "shared/include/packet_writer.hpp"

using namespace asio::ip;


// Depending on the server admin, 
// they are able to host game assets needed to join the server.
// This is used to download game assets, models, textures and audio.



namespace AM {
    
   
    class AssetsDownloader {
        public:

            AssetsDownloader(
                    const AM::ClientConfig& config,
                    asio::io_context& context,
                    const char* host,
                    const char* port);

            void update_assets();
            void close_connection(asio::io_context& context);

        private:

            std::atomic<bool> m_keep_connection_alive { true };

            struct mAssetFile {
                std::string name;
                std::string type; // "model", "texture" or "audio"
                std::string sha256_hash;
            };

            AM::Packet m_packet;
            void m_send_packet();

            std::thread m_event_handler_th;

            void m_write_data(const std::string& data);
            asio::ip::tcp::socket m_tcp_socket;
            void m_do_read_tcp();

            void m_handle_recv_data(size_t size);
            char m_recv_data[AM::MAX_PACKET_SIZE] { 0 };

            void m_find_local_gamefiles(
                    std::vector<mAssetFile>* files,
                    const std::string& path,
                    const std::string& asset_type);
           
            size_t m_download_sizeb { 0 };
            size_t m_downloaded_bytes { 0 };
            std::string   m_download_filename;
            std::ofstream m_download_file;

            AM::ClientConfig m_config;
    };

};




#endif
