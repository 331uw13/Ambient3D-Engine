#ifndef AMBIENT3D_TCP_SESSION_HPP
#define AMBIENT3D_TCP_SESSION_HPP


#include <memory>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <asio.hpp>

using json = nlohmann::json;
using namespace asio::ip;


// From '../../shared'
#include "packet_writer.hpp"
#include "client_config.hpp"

namespace AM {
    class Server;

    class TCP_session : public std::enable_shared_from_this<TCP_session> {
        public:
            TCP_session(tcp::socket socket, AM::Server* server, int player_id);
            void start() { m_do_read(); }


            Packet packet;
            void send_packet();
            int  player_id;
            
            bool is_fully_connected() { return m_fully_connected; }

            AM::ClientConfig config;

        private:

            std::vector<std::string> m_write_buffer;

            void m_do_read();

            tcp::socket  m_socket;
            AM::Server*  m_server;

            void m_handle_received_packet(size_t sizeb);
            char m_data[AM::MAX_PACKET_SIZE]; // <- Received data. (TODO: Should probably rename.)
            bool m_fully_connected { false };
    };
};



#endif
