#ifndef AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP
#define AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP

#include <memory>
#include <asio.hpp>
#include <deque>

#include "config.hpp"
#include "asset_files.hpp"
#include "shared/include/networking_agreements.hpp"
#include "shared/include/packet_writer.hpp"

using namespace asio::ip;



namespace AM {


    class TCP_session : public std::enable_shared_from_this<TCP_session> {
        public:
            TCP_session(const AM::Config& config, AM::AssetFileStorage* file_storage, tcp::socket socket) : 
                m_config(config), 
                m_file_storage(file_storage),
                m_socket(std::move(socket)) {};


            AM::Packet packet;
            void send_packet();

            void start();
            //void write_bytes(char* bytes, size_t sizeb);

        private:

            AM::Config             m_config;
            AM::AssetFileStorage*  m_file_storage;
            tcp::socket            m_socket;


            std::deque<AM::AssetFile> m_download_queue;
            void    m_do_read();
            void    m_handle_recv_data(size_t size);

            void    m_send_download_queue_next_fileinfo();
            void    m_read_next_file_for_sending();
            void    m_send_download_queue_next_bytes();

            char*   m_current_file_bytes { NULL };
            size_t  m_current_file_size { 0 };
            size_t  m_current_file_byteoffset { 0 };
            bool    m_current_file_complete { false };

            bool    m_match_client_filehash(
                    const json& client_filehashes_json, const AM::AssetFile& file);

            // Packet data. TODO: Rename,
            char m_data[AM::MAX_PACKET_SIZE] { 0 };
    };

};




#endif
