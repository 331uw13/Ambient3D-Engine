#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>

#include <nlohmann/json.hpp>

#include "tcp_session.hpp"
#include "config.hpp"
#include "shared/include/packet_parser.hpp"

using json = nlohmann::json;




void AM::TCP_session::m_send_download_queue_next_fileinfo() {
    json fileinfo = json::parse("{}");

    AM::AssetFile& file = m_download_queue.front();

    fileinfo["filename"] = file.name;
    fileinfo["filesize"] = file.size;
    fileinfo["filegroup"] = file.type_group;


    this->packet.prepare(AM::PacketID::CREATE_ASSET_FILE);
    this->packet.write_string({ fileinfo.dump() });
    this->send_packet();
}


void AM::TCP_session::m_read_next_file_for_sending() {
    AM::AssetFile& file = m_download_queue.front();

    std::ifstream current_file(file.full_path, std::ios::in | std::ios::binary | std::ios::ate);
    if(!current_file.is_open()) {
        fprintf(stderr, "%s: Failed to open '%s'\n", __func__, file.full_path.c_str());

        // TODO: Inform client there was error on server side and abort download process.
        return;
    }

    if(m_current_file_bytes) {
        delete[] m_current_file_bytes;
        m_current_file_bytes = NULL;
    }

    m_current_file_size = current_file.tellg();
    m_current_file_bytes = new char[m_current_file_size];

    current_file.seekg(0, std::ios::beg);


    current_file.read(m_current_file_bytes, m_current_file_size);

    printf("'%s' First 16 bytes: \033[32m", file.name.c_str());
    for(size_t i = 0; i < 16; i++) {
        printf("%02X ", (uint8_t)m_current_file_bytes[i]);
    }
    printf("\033[0m\n");

    current_file.close();

    m_current_file_complete = false;
    m_current_file_byteoffset = 0;
}

void AM::TCP_session::m_send_download_queue_next_bytes() {

    if(m_current_file_complete) {
        m_download_queue.pop_front();
        if(m_download_queue.empty()) {
            this->packet.prepare(AM::PacketID::ASSET_FILE_END);
            this->send_packet();
            return;
        }
        m_send_download_queue_next_fileinfo();
        return;
    }

    int64_t block_size = 1024;

    if(m_current_file_byteoffset + block_size >= m_current_file_size) {
        m_current_file_complete = true;
        block_size = (m_current_file_size - m_current_file_byteoffset); // Remaining bytes.
    }
    
    this->packet.prepare(AM::PacketID::ASSET_FILE_BYTES);
    this->packet.write_bytes(
            (void*)&m_current_file_bytes[m_current_file_byteoffset],
            block_size
            );
    this->send_packet();
    
    printf(" -> (block_size=%li) (offset=%li / %li)\n", block_size, m_current_file_byteoffset, m_current_file_size); 
    m_current_file_byteoffset += block_size;


    if(m_current_file_byteoffset >= m_current_file_size) {
        printf("COMPLETE!\n");

        m_current_file_complete = true;

    }


}


void AM::TCP_session::m_handle_recv_data(size_t size) {

    AM::PacketID packet_id = AM::parse_network_packet(m_data, size);

    switch(packet_id) {

        case AM::PacketID::CLIENT_GAMEASSET_FILE_HASHES:
            
            // Respond with files which need downloading.
            try {
                json client_file_hashes = json::parse(m_data);
                
                int file_counter = 0;
                /*
                for(const json& j : client_file_hashes) {
                    
                    // TODO: Check the hash and if matches add it to download queue.

                    file_counter++;
                }*/

                //if(file_counter == 0) {
                    // Add everything in file storage.
                    // Client doesnt seem to have any files downloaded.
                    for(size_t i = 0; i < m_file_storage->texture_files.size(); i++) {
                        m_download_queue.push_back(m_file_storage->texture_files[i]);
                    }
                    for(size_t i = 0; i < m_file_storage->model_files.size(); i++) {
                        m_download_queue.push_back(m_file_storage->model_files[i]);
                    }
                    for(size_t i = 0; i < m_file_storage->audio_files.size(); i++) {
                        m_download_queue.push_back(m_file_storage->audio_files[i]);
                    }
                //}

                size_t total_download_bytes = 0;
                json files_json = json::parse("{}");

                for(size_t i = 0; i < m_download_queue.size(); i++) {
                    AM::AssetFile& file = m_download_queue[i];
                    files_json["files"][i][0] = file.name;
                    files_json["files"][i][1] = file.size;

                    total_download_bytes += file.size;
                }

                printf("CLIENT DOWNLOAD QUEUE:\n%s\n", files_json.dump(4).c_str());

                this->packet.prepare(AM::PacketID::DO_ACCEPT_ASSETS_DOWNLOAD);
                this->packet.write<size_t>({ total_download_bytes });
                this->packet.write_string({ files_json.dump() });
                this->send_packet();
            }
            catch(const std::exception& e) {
                fprintf(stderr, "[CLIENT_GAMEASSET_FILE_HASHES]: %s\n", e.what());
            }
            break;

        case AM::PacketID::ACCEPTED_ASSETS_DOWNLOAD:
            printf("Client accepted download.\n");
            m_send_download_queue_next_fileinfo();
            break;

        case AM::PacketID::CLIENT_CREATED_ASSET_FILE:
            printf("Client has created the asset file. Begin download process\n");
            m_read_next_file_for_sending();
            m_send_download_queue_next_bytes();
            break;

        case AM::PacketID::GOT_SOME_FILE_BYTES:
            m_send_download_queue_next_bytes();
            break;
    }


}
            
void AM::TCP_session::start() {
    this->packet.allocate_memory();
    m_do_read();
}


void AM::TCP_session::m_do_read() {
    
    memset(m_data, 0, AM::MAX_PACKET_SIZE);

    //const std::shared_ptr<TCP_session>& self(shared_from_this());
    m_socket.async_read_some(asio::buffer(m_data, AM::MAX_PACKET_SIZE),
            [this](std::error_code ec, std::size_t size) {
                if(ec) {
                    printf("[read](%i): %s\n", ec.value(), ec.message().c_str());
                    this->packet.free_memory();
                    if(m_current_file_bytes) {
                        delete[] m_current_file_bytes;
                    }
                    
                    // TODO: Remove client.
                
                }
                else {
                    m_handle_recv_data(size);
                }

                m_do_read();
            });
}


void AM::TCP_session::send_packet() {
    if((this->packet.get_flags() & AM::Packet::FLG_WRITE_ERROR)) {
        return;
    }

    asio::async_write(m_socket, asio::buffer(this->packet.data, this->packet.size),
            [this](std::error_code ec, std::size_t /*size*/) {
                if(ec) {
                    printf("[write](%i): %s\n", ec.value(), ec.message().c_str());
                    this->packet.free_memory();
                    if(m_current_file_bytes) {
                        delete[] m_current_file_bytes;
                    }

                    // TODO: Remove client.

                    return;
                }
            });

    this->packet.enable_flag(AM::Packet::FLG_COMPLETE);
}


