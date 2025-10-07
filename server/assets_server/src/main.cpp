#include "server.hpp"





int main(int argc, char** argv) {

    asio::io_context io_context;
    AM::GameAssetsServer server(io_context, 34470);
    server.start(io_context);

    return 0;
}
