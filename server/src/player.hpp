#ifndef AMBIENT3D_SERVER_PLAYER_HPP
#define AMBIENT3D_SERVER_PLAYER_HPP

#include <unordered_map>

#include "tcp_session.hpp"
#include "vec3.hpp"
#include "terrain/chunk.hpp"


namespace AM {
    class Server;

    class Player {
        public:

            Player(std::shared_ptr<AM::TCP_session> _tcp_session);
           
            std::shared_ptr<AM::TCP_session> tcp_session;
            std::unordered_map<AM::ChunkPos, bool> loaded_chunks;

            int    id           { -1 };
            Vec3   pos          { 0, 0, 0 };
            float  cam_yaw      { 0 };
            float  cam_pitch    { 0 };
            int    anim_id      { 0 };
            bool   on_ground    { true };
            bool   pos_xz_axis_updated { false };

            float terrain_surface_y { 0 };
            void update_gravity(float gravity);

        private:

    };


};


#endif
