#include "server.hpp"





int main(int argc, char** argv) {


    AM::Config config("config.json");
    AM::AssetFileStorage file_storage;

    AM::find_asset_files(config, &file_storage);
    AM::compute_asset_file_hashes(&file_storage);

    asio::io_context io_context;
    AM::GameAssetsServer server(config, io_context);
    server.start(io_context);

    return 0;
}
