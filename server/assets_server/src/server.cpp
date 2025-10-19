#include "server.hpp"






AM::GameAssetsServer::GameAssetsServer(
        const AM::Config& config,
        AM::AssetFileStorage* file_storage,
        asio::io_context& context
) :
    m_file_storage(file_storage),
    m_tcp_acceptor(context, tcp::endpoint(tcp::v4(), config.port)) {
   
    m_config = config;
}

AM::GameAssetsServer::~GameAssetsServer() {

}


void AM::GameAssetsServer::start(asio::io_context& context) {
    printf("Ambient3D - Game assets server started.\n");
    m_do_accept_tcp();
    context.run();
}


void AM::GameAssetsServer::m_do_accept_tcp() {
    m_tcp_acceptor.async_accept([this](std::error_code ec, tcp::socket socket) {
        if(ec) {
            printf("[accept](%i): %s\n", ec.value(), ec.message().c_str());
            m_do_accept_tcp();
            return;
        }

        printf("Client connected.\n");

        m_clients.push_back(std::make_shared<AM::TCP_session>(m_config, m_file_storage, std::move(socket)));
        m_clients.back()->start();

        m_do_accept_tcp();
    });
}


