#ifndef AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP
#define AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP

#include <memory>
#include <asio.hpp>

#include "config.hpp"
#include "../../../shared/networking_agreements.hpp"

using namespace asio::ip;



namespace AM {


    class TCP_session : public std::enable_shared_from_this<TCP_session> {
        public:
            TCP_session(const AM::Config& config, tcp::socket socket) 
                : m_config(config), m_socket(std::move(socket)) {};
            void start() { m_do_read(); }

            void write_bytes(char* bytes, size_t sizeb);

        private:

            AM::Config    m_config;
            tcp::socket   m_socket;
            void          m_do_read();

            void m_handle_received_packet(size_t size);
            char m_data[AM::MAX_PACKET_SIZE] { 0 };
    };

};




#endif
