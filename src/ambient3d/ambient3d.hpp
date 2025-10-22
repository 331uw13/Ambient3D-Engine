#ifndef AMBIENT3D_HPP
#define AMBIENT3D_HPP

#include "raylib.h"
#include <cstdint>
#include <map>
#include <array>
#include <functional>
#include <unordered_map>

#include "player.hpp"
#include "shader_util.hpp"
#include "uniform_buffer.hpp"
#include "light.hpp"
#include "renderable.hpp"
#include "glsl_preproc.hpp"
#include "util.hpp"
#include "timer.hpp"
#include "network/network.hpp"
#include "terrain/terrain.hpp"
#include "transparent_str_hash.hpp"
#include "shared/include/client_config.hpp"


namespace AM {

    //static constexpr int NUM_BLOOM_SAMPLES = 16;
    static constexpr int MAX_LIGHTS = 64;
    static constexpr int CHAT_KEY = KEY_ENTER;
    static constexpr Vector3 UP_VECTOR = Vector3(0.0f, 1.0f, 0.0f);
    static constexpr float FRICTION_TCONST = 120.0f;

    static constexpr float FAST_FIXED_TICK_DELAY_SECONDS = 0.075f;
    static constexpr float SLOW_FIXED_TICK_DELAY_SECONDS = 4.0f;

    namespace ShaderIDX {
        enum : int {
            DEFAULT,
            DEFAULT_INSTANCED,
            POST_PROCESSING,
            BLOOM_TRESHOLD,
            BLOOM_DOWNSAMPLE_FILTER,
            BLOOM_UPSAMPLE_FILTER,
            SKYBOX,
            SINGLE_COLOR,

            NUM_SHADERS
            // ...
        };
    };

    namespace MaterialIDX {
        enum : int {

            // ...

            NUM_MATERIALS,
        };
    }

    namespace StateFlags {

        static constexpr int DONT_RENDER_DEFAULT_ITEMINFO = (1 << 0);
    };

    class State {
        public:

            State(){};
            State(uint16_t win_width, uint16_t win_height,
                    const char* title,
                    const AM::ClientConfig& config,
                    AM::NetConnectCFG network_cfg);

            ~State();

            void enable_flag(int state_flag)  { m_flags |=  state_flag; }
            void disable_flag(int state_flag) { m_flags &= ~state_flag; }

            Font         font;
            Player       player;
            Terrain      terrain;
            ItemManager  item_manager;
            Network*     net;
            ClientConfig config;

            // Set to true when complete connection is ready.
            std::atomic<bool> ready { false };

            void    frame_begin();
            void    frame_end();


            // === SHADERS ===
            // Shaders in this array will be unloaded when the state is destructed.
            
            std::array<Shader, AM::ShaderIDX::NUM_SHADERS> shaders;
            void set_shader(int shader_idx, const Shader& shader);

            // === GENERAL PURPOSE MATERIALS ===
            // Materials in this array will be unloaded when the state is destructed.

            std::array<Material, AM::MaterialIDX::NUM_MATERIALS> materials;
            void set_material(int material_idx, const Material& material);



            // ===  LIGHTS ===

            Light** add_light(const Light& light);
            void    remove_light(Light** light);
            void    update_lights();


            // === NAMED TIMERS ===
            // Timers are updated on 'AM::State::frame_end()' 

            void    create_named_timer(const std::string_view& timer_name);
            void    delete_named_timer(const std::string_view& timer_name);
            AM::Timer* get_named_timer(const std::string_view& timer_name);


            // === MOVEMENT CONTROL ===

            void    set_mouse_enabled(bool enabled);
            bool    is_mouse_enabled() { return m_mouse_enabled; }

            void    set_movement_enabled(bool enabled) { m_movement_enabled = enabled; }
            bool    is_movement_enabled() { return m_movement_enabled; }


            // === MISC ===
          
            // Time of day is in range of 0.0 to 1.0
            // 0.0 is mid night. 0.5 is mid day and 1.0 is mid night again.
            float   raw_timeofday { 0.0f };
            // Time of day curve smooths out the cycle so that
            // when raw_timeofday is near 0.0(mid night) return value is near 0.0
            // when raw_timeofday is near 0.5(mid day) return value is near 1.0
            // when raw_timeofday is near 1.0(mid night again) return value will be near 0.0 again.
            float   timeofday_curve { 0.0f };

            void    draw_info();
            void    draw_text(int font_size, const char* text, int x, int y, const Color& color);
            
            void    draw_text_3D(
                    const char* text,
                    float font_scale,
                    float font_spacing,
                    Color color, 
                    Vector2 originxy,
                    Vector3 position,
                    Vector3 rotation);

            //      When amount is close to 0.0 small distortions happen
            //      but when it reaches 0.5 "blinking" starts happening
            //      and it gets stronger towards 1.0
            void    set_vision_effect(float amount);


            void set_fast_fixed_tick_callback(std::function<void(AM::State*)> callback) {
                m_fast_fixed_tick_callback = callback;
            }
            
            void set_slow_fixed_tick_callback(std::function<void(AM::State*)> callback) {
                m_slow_fixed_tick_callback = callback;
            }

        private:

            int m_flags { 0 };

            bool m_mouse_enabled          { true };
            bool m_movement_enabled       { true };
            bool m_connected_to_server    { false };
            bool m_is_chat_open           { false };

            asio::io_context m_asio_io_context;

            enum RenderTargetIDX : int {
                RESULT,
                BLOOM_TRESHOLD,
                BLOOM_RESULT,

                NUM_TARGETS
            };

            Model m_skybox_model;

            std::unordered_map<std::string, std::shared_ptr<AM::Timer>, 
                AM::TransparentStringHash, std::equal_to<>> m_named_timers;
            std::mutex                                      m_named_timers_mutex;
            
            std::array<RenderTexture2D, RenderTargetIDX::NUM_TARGETS>
                m_render_targets;

            void                             m_update_dropped_items();
            void                             m_render_default_iteminfo(const AM::Item* item);
            void                             m_render_skybox();

            std::function<void(AM::State*)>  m_slow_fixed_tick_callback;
            std::function<void(AM::State*)>  m_fast_fixed_tick_callback;
            void                             m_slow_fixed_tick_update();
            void                             m_fast_fixed_tick_update();
            void                             m_set_shader_uniforms();
            void                             m_update_timeofday();
            
            void                             m_update_player();

            void                             m_create_internal_timers();

            // TODO Maybe move this away from State class
            void m_render_bloom();

    

            UniformBuffer m_lights_ubo;
            std::array<Light, MAX_LIGHTS> m_lights;
            std::array<Light*, MAX_LIGHTS> m_light_ptrs { NULL };
            size_t m_num_lights { 0 };

            std::map<int/*light ID*/, Light>        m_lights_pframe_map; // Previous frame lights.
       


    };

};




#endif
