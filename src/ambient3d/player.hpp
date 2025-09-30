#ifndef AMBIENT3D_PLAYER_HPP
#define AMBIENT3D_PLAYER_HPP

#include <cstdint>
#include <atomic>

#include "raylib.h"
#include "rcamera.h"

#include "anim_ids.hpp"
#include "threadsafe_stack.hpp"
#include "chunk_pos.hpp"

namespace AM {
    class State;
    class Player {
        public:
            Player();
            ~Player();
            
            Camera camera;
           
            void set_engine_state(AM::State* st) {
                m_engine = st;
            }

            
            Vector3 position() const;                  // < thread safe >
            void    set_position(const Vector3& pos);  // < thread safe >

            Vector3 velocity() const;                  // < thread safe >
            void    set_velocity(const Vector3& vel);  // < thread safe >

            float   walking_speed() const;             // < thread safe >
            void    set_walking_speed(float speed);    // < thread safe >

            float   running_speed() const;             // < thread safe >
            void    set_running_speed(float speed);    // < thread safe >

            float   movement_friction() const;         // < thread safe >
            void    set_movement_friction(float f);    // < thread safe >

            Vector3 camera_forward() const;            // < thread safe >
            Vector3 camera_right() const;              // < thread safe >

            float   camera_yaw() const;                // < thread safe >
            void    set_camera_yaw(float yaw);         // < thread safe >

            float   camera_pitch() const;              // < thread safe >
            void    set_camera_pitch(float pitch);     // < thread safe >

            AM::AnimID   animation_id() const;                    // < thread safe >
            void         set_animation_id(AM::AnimID anim_id);    // < thread safe >

            AM::ChunkPos chunk_pos() const;                       // < thread safe >
            void    set_chunk_pos(const AM::ChunkPos& chunk_pos); // < thread safe >
            

            float camera_sensetivity() const;
            void  set_camera_sensetivity(float sensetivity);
            
            float camera_fov() const;
            void  set_camera_fov(float fov);

            void update_position_from_server();                  // < thread safe >
            void update_movement(bool handle_user_input = true); // < thread safe >
            void update_camera();                                // < thread safe >
            void update_animation();

            void jump(); // < thread safe >

            std::atomic<bool> on_ground { false };
            std::atomic<bool> fully_connected { false };

            // Position stack sent from server side.
            // Latest one is at first index.
            // Y and XZ are separeted because sometimes XZ position is not needed to update.
            AM::Stackts<float>    Y_pos_update_stack; // < thread safe >
            AM::Stackts<Vector2> XZ_pos_update_stack; // < thread safe >
            
            // Few previous frame positions handled by the client.
            // See size limit from player class constructor.
            // Latest one is at first index.
            AM::Stackts<Vector3> prevframe_positions;  // < thread safe >

        private:

            mutable std::mutex m_mutex;
            
            Vector3 m_movement { 0.0f, 0.0f, 0.0f }; 

            // IMPORTANT NOTE:
            // It is more safe to access these variables through
            // public set/get functions because they are thread safe.

            Vector3 m_position { 0.0f, 0.0f, 0.0f };
            Vector3 m_velocity { 0.0f, 0.0f, 0.0f };
            float   m_camera_yaw { 0.0f };
            float   m_camera_pitch { 0.0f };
            float   m_camera_sensetivity { 0.0f };
            Vector3 m_camera_right { 0.0f, 0.0f, 0.0f };
            Vector3 m_camera_forward { 0.0f, 0.0f, 0.0f };
            float m_movement_friction { 0.97f };   // TODO: Get this from server.
            float m_walking_speed { 30.0f };      // TODO: Get this from server.
            float m_running_speed { 800.0f };      // TODO: Get this from server.
            float m_current_move_speed { 0.0f };
            AM::AnimID m_animation_id { AM::AnimID::IDLE };
            void m_update_Y_axis_position();
            void m_update_XZ_axis_position();
            AM::State* m_engine;
            AM::ChunkPos m_chunk_pos;
    };
};




#endif
