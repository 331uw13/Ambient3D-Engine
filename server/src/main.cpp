#include <cstdio>
#include <cstdlib>

#include "server.hpp"

    
//constexpr const char* item_list_path = "../items/item_list.json";



int main() {
    printf("Ambient3D - Server\n");

    asio::io_context io_context;
    AM::ServerCFG config("server_config.json");
    AM::Server server(io_context, config);
}

