#ifndef AMBIENT3D_GAME_ASSETS_SERVER_HPP
#define AMBIENT3D_GAME_ASSETS_SERVER_HPP

#include "tcp_session.hpp"
#include "config.hpp"
#include "asset_files.hpp"


namespace AM {

    class GameAssetsServer {
        public:

            GameAssetsServer(const AM::Config& config, asio::io_context& context);
            ~GameAssetsServer();

            void start(asio::io_context& context);



        private:

            std::vector<std::shared_ptr<AM::TCP_session>> m_clients;

            AM::Config     m_config;
            void           m_do_accept_tcp();
            tcp::acceptor  m_tcp_acceptor;
    };


};


#endif
