#ifndef AMBIENT3D_GAME_ASSETS_DOWNLOADER_HPP
#define AMBIENT3D_GAME_ASSETS_DOWNLOADER_HPP

#include <asio.hpp>
#include "shared/include/networking_agreements.hpp"

using namespace asio::ip;


// Depending on the server admin, 
// they are able to host game assets needed to join the server.
// This is used to download game assets, models, textures and audio.



namespace AM {

    class AssetsDownloader {
        public:

            AssetsDownloader(asio::io_context& context, const char* host, const char* port);


        private:


            void m_write_data(const std::string& data);
            asio::ip::tcp::socket m_tcp_socket;
            void m_do_read_tcp();
            char m_recv_data[AM::MAX_PACKET_SIZE] { 0 };

    };

};




#endif
