#ifndef AMBIENT3D_GAME_ASSETS_SERVER_HPP
#define AMBIENT3D_GAME_ASSETS_SERVER_HPP

#include <asio.hpp>
#include "tcp_session.hpp"

using namespace asio::ip;




namespace AM {

    class GameAssetsServer {
        public:

            GameAssetsServer(asio::io_context& context, uint16_t port);
            ~GameAssetsServer();

            void start(asio::io_context& context);



        private:

            std::vector<std::shared_ptr<AM::TCP_session>> m_clients;

            void m_do_accept_tcp();
            tcp::acceptor m_tcp_acceptor;
    };


};


#endif
