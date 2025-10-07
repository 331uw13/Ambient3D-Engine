#include <cstdio>

#include "assets_downloader.hpp"




            

AM::AssetsDownloader::AssetsDownloader(asio::io_context& context, const char* host, const char* port)
: m_tcp_socket(context) {
    try {

        asio::ip::tcp::resolver tcp_resolver(context);
        asio::connect(m_tcp_socket, tcp_resolver.resolve(host, port));

        m_write_data("HELLO?????????\n");

        m_do_read_tcp();
    }
    catch(const std::exception& e) {
        fprintf(stderr, "[AssetsDownloader]: %s\n", e.what());
    }
}


void AM::AssetsDownloader::m_write_data(const std::string& data) {
    asio::async_write(m_tcp_socket, asio::buffer(data, data.size()),
            [this](std::error_code ec, std::size_t /*size*/) {
                if(ec) {
                    fprintf(stderr, "[AssetsDownloader write](%i): %s\n", ec.value(), ec.message().c_str());
                }
            });
}


void AM::AssetsDownloader::m_do_read_tcp() {

    memset(m_recv_data, 0, AM::MAX_PACKET_SIZE);
    m_tcp_socket.async_read_some(asio::buffer(m_recv_data, AM::MAX_PACKET_SIZE),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    fprintf(stderr, "[AssetsDownloader read](%i): %s\n", ec.value(), ec.message().c_str());
                    return;
                }

                printf("%s\n", m_recv_data);

            });

}





