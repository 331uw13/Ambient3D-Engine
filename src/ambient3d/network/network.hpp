#ifndef AMBIENT3D_NETWORKING_HPP
#define AMBIENT3D_NETWORKING_HPP

#include <cstdint>
#include <thread>
#include <functional>
#include <map>
#include <atomic>
#include <nlohmann/json.hpp>

#include <deque>
#include <asio.hpp>

#include "packet_writer.hpp"
#include "network_player.hpp"
#include "server_config.hpp"
#include "net_dynamic_data.hpp"
#include "../item_manager.hpp"
#include "../terrain/terrain.hpp"

#include "../gui/chatbox.hpp"

using json = nlohmann::json;


namespace AM {

    struct NetConnectCFG {
        const char* host;
        const char* tcp_port;
        const char* udp_port;

        // Called when data is received.
        std::function<void(
                uint8_t, // Red
                uint8_t, // Green
                uint8_t, // Blue
                const std::string&)> msg_recv_callback { NULL };
    };

    enum NetProto {
        TCP,
        UDP
    };


    struct PacketInterval {
        float latest_interval_ms;
        float begin_time_sc;
    };

    struct PacketCallbacks {
        std::vector<std::function<void(float /*interval_ms*/, char* /*data*/, size_t/*sizeb*/)>> functions;
    };

    class State;
    class Network {

        public:

            Network(asio::io_context& io_context, const NetConnectCFG& cfg);
            ~Network() {}
            void close(asio::io_context& io_context);

            void set_engine_state(AM::State* state) {
                m_engine = state;
            }


            // The packet can be written with functions from
            // './shared/packet_writer.*'
            // This is going to be saved here so it dont need to be
            // always allocated again.
            // TODO: Make packet thread safe!
            Packet packet;
            void   send_packet(AM::NetProto proto);

            // Handles settings/data which may change overtime 
            // sent by the server.
            AM::NetworkDynamicData  dynamic_data;  // < thread safe >

            std::mutex                                 players_mutex;
            std::map<int /*player_id*/, AM::N_Player>  players;

            void foreach_online_players(std::function<void(AM::N_Player*)> callback); // < thread safe >

            bool is_connected()        { return m_connected; }       // < thread safe >
            bool is_fully_connected()  { return m_fully_connected; } // < thread safe >


            // IMPORANT NOTE:
            // The callbacks are called from 
            // network event handler thread and not the main thread.

            void add_packet_callback  // < thread safe >
                (AM::NetProto protocol, AM::PacketID packet_id, 
                    std::function<void(float /*interval_ms*/, char* /*data*/, size_t /*sizeb*/)> callback);

            float get_packet_interval_ms(AM::PacketID packet_id); // < thread safe >


            int           player_id;
            AM::ServerCFG server_cfg;
       
            std::atomic<bool> needto_sync_timeofday { false };

        private:

            void m_attach_main_TCP_packet_callbacks();
            void m_attach_main_UDP_packet_callbacks();


            void m_attach_main_callbacks();

            std::atomic<bool> m_connected { false };
            std::atomic<bool> m_fully_connected { false };

            std::mutex          m_packet_callbacks_mutex;
            AM::PacketCallbacks m_tcp_packet_callbacks [AM::PacketID::NUM_PACKETS];
            AM::PacketCallbacks m_udp_packet_callbacks [AM::PacketID::NUM_PACKETS];

            void m_call_packet_callbacks(AM::NetProto protocol, AM::PacketID packet_id, size_t packet_sizeb);
            
            std::mutex          m_packet_intervals_mutex;
            void                m_update_packet_interval(AM::PacketID packet_id);
            AM::PacketInterval  m_packet_intervals     [AM::PacketID::NUM_PACKETS];


            std::function<void(
                    uint8_t, // Red
                    uint8_t, // Green
                    uint8_t, // Blue
                    const std::string&)> m_msg_recv_callback;

            AM::State* m_engine;
                
            bool m_udp_data_ready_to_send;
            bool m_tcp_data_ready_to_send;
           
            std::thread m_event_handler_th;

            asio::ip::tcp::socket m_tcp_socket;
            void m_do_write_tcp();
            void m_do_read_tcp();


            asio::ip::udp::socket m_udp_socket;
            asio::ip::udp::endpoint m_udp_sender_endpoint;
            void m_do_read_udp();
            void m_do_write_udp();

            char m_udprecv_data[AM::MAX_PACKET_SIZE];
            char m_tcprecv_data[AM::MAX_PACKET_SIZE];
            
            void m_handle_tcp_packet(size_t sizeb);
            void m_handle_udp_packet(size_t sizeb);
    };
};


#endif
