#ifndef AMBIENT3D_SERVER_PLAYER_HPP
#define AMBIENT3D_SERVER_PLAYER_HPP

#include <unordered_map>
#include <mutex>
#include <atomic>

#include "tcp_session.hpp"
#include "terrain/chunk.hpp"
#include "shared/include/inventory.hpp"
#include "shared/include/vec3.hpp"



namespace AM {
    class Server;

    class Player {
        public:

            Player(std::shared_ptr<AM::TCP_session> _tcp_session);
            Player(){}
            std::shared_ptr<AM::TCP_session> tcp_session;
            std::unordered_map<AM::ChunkPos, bool> loaded_chunks;

            void free_memory();

            void set_server(AM::Server* server) { 
                m_server = server;
            }

            std::mutex    inventory_mutex;
            AM::Inventory inventory;

            int id() const;                      // < thread safe >
            void set_id(int id);                 // < thread safe >
            
            float cam_yaw() const;               // < thread safe >
            void set_cam_yaw(float yaw);         // < thread safe >

            float cam_pitch() const;             // < thread safe >
            void set_cam_pitch(float pitch);     // < thread safe >

            float terrain_surface_y() const;     // < thread safe >
            AM::ChunkPos chunk_pos() const;      // < thread safe >

            AM::Vec3 position() const;            // < thread safe >
            void set_position(const AM::Vec3& p); // < thread safe >

            AM::Vec3 velocity() const;            // < thread safe >
            void set_velocity(const AM::Vec3& v); // < thread safe >

            int animation_id() const;             // < thread safe >
            void set_animation_id(int id);        // < thread safe >
            
            // Used for server to control player's positions.
            // When AM::Server::m_send_player_position() is called
            // from the update thread it will use this as the position to send.
            // The client will then adjust its position how instructed.
            AM::Vec3 next_position() const;                // < thread safe >
            void set_next_position_Y(float y);             // < thread safe >
            void set_next_position_XYZ(const AM::Vec3& p); // < thread safe >
            int next_position_flags() const;               // < thread safe >
            void clear_next_position_flags();              // < thread safe >

            void update();  // < thread safe >

            std::atomic<bool> on_ground       { true };

        private:

            mutable std::mutex m_mutex;

            int m_next_position_flags { 0 };
            int m_id { -1 };
            AM::Vec3 m_position { 0.0f, 0.0f, 0.0f };
            AM::Vec3 m_next_position { 0.0f, 0.0f, 0.0f };
            AM::Vec3 m_velocity { 0.0f, 0.0f, 0.0f };
            AM::ChunkPos m_chunk_pos = {};
            float m_cam_yaw { 0.0f };
            float m_cam_pitch { 0.0f };
            int m_animation_id { 0 };
            float m_terrain_surface_y { 0.0f };
            
            AM::Server* m_server { NULL };

            void m_update_gravity();

    };


};


#endif
