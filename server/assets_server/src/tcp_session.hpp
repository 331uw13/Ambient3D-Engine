#ifndef AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP
#define AMBIENT3D_GAME_ASSETS_TCP_SESSION_HPP

#include <memory>
#include <asio.hpp>

using namespace asio::ip;


#include "config.hpp"


namespace AM {


    class TCP_session : public std::enable_shared_from_this<TCP_session> {
        public:
            TCP_session(tcp::socket socket) : m_socket(std::move(socket)) {};
            void start() { m_do_read(); }

        private:

            void m_do_read();
            tcp::socket  m_socket;

            void m_handle_received_packet(size_t size);
            char m_data[Config::MAX_PACKET_SIZE+1] { 0 };
    };

};




#endif
