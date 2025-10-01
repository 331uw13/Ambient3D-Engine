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
#include "client_config.hpp"
#include "network/network.hpp"
#include "terrain/terrain.hpp"
#include "transparent_str_hash.hpp"


namespace AM {

    static constexpr int NUM_BLOOM_SAMPLES = 16;
    static constexpr int MAX_LIGHTS = 64;
    static constexpr int CHAT_KEY = KEY_ENTER;
    static constexpr Vector3 UP_VECTOR = Vector3(0.0f, 1.0f, 0.0f);
    static constexpr float FRICTION_TCONST = 120.0f;

    static constexpr float FAST_FIXED_TICK_DELAY_SECONDS = 0.075f;
    static constexpr float SLOW_FIXED_TICK_DELAY_SECONDS = 4.0f;

    enum ShaderIDX : int {
        DEFAULT,
        DEFAULT_INSTANCED,
        POST_PROCESSING,
        BLOOM_TRESHOLD,
        BLOOM_DOWNSAMPLE_FILTER,
        BLOOM_UPSAMPLE_FILTER,
        // ...
    };

    enum GuiModuleFocus {
        GAIN,
        LOSE,
        TOGGLE
    };

    class State {
        public:
            State(uint16_t win_width, uint16_t win_height,
                    const char* title,
                    const char* config_file,
                    AM::NetConnectCFG network_cfg);

            ~State();

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
            std::vector<Shader> shaders;
            void add_shader(const Shader& shader);


            // ===  LIGHTS ===

            Light** add_light(const Light& light);
            void    remove_light(Light** light);
            void    update_lights();


            // === GUI MODULES ===

            template<class MODULE>
            void register_gui_module(
                    AM::GuiModuleID module_id, // User chosen ID.
                    AM::GuiModule::RenderOPT render_option
            ){
                m_gui_modules.push_back(
                        std::make_unique<MODULE>(MODULE(module_id, render_option)));
            }

            template<class MODULE>
            MODULE* find_gui_module(AM::GuiModuleID module_id) {
                for(size_t i = 0; i < m_gui_modules.size(); i++) {
                    if(m_gui_modules[i]->get_id() == module_id) {
                        return dynamic_cast<MODULE*>(m_gui_modules[i].get());
                    }
                }
                return NULL;
            }

            void    set_gui_module_focus(int module_id, GuiModuleFocus focus_option);


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
            
            void    draw_info();
            void    draw_text(int font_size, const char* text, int x, int y, const Color& color);

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
            bool m_mouse_enabled          { true };
            bool m_movement_enabled       { true };
            bool m_connected_to_server    { false };
            bool m_is_chat_open           { false };

            asio::io_context m_asio_io_context;

            enum RenderTargetIDX : int {
                RESULT,
                BLOOM_TRESHOLD,
                BLOOM_PRE_RESULT,
                BLOOM_RESULT,

                NUM_TARGETS
            };


        std::unordered_map<std::string, std::shared_ptr<AM::Timer>, 
            AM::TransparentStringHash, std::equal_to<>> m_named_timers;
        std::mutex                                      m_named_timers_mutex;

        std::array<RenderTexture2D, RenderTargetIDX::NUM_TARGETS>
            m_render_targets;

        std::array<RenderTexture2D, NUM_BLOOM_SAMPLES>
            m_bloom_samples;

        void                             m_render_dropped_items();

        //float                            m_fixed_tick_timer         { 0.0f };
        //float                            m_fixed_tick_delay         { 0.075f };
        std::function<void(AM::State*)>  m_slow_fixed_tick_callback;
        std::function<void(AM::State*)>  m_fast_fixed_tick_callback;
        void                             m_slow_fixed_tick_update();
        void                             m_fast_fixed_tick_update();
        
        void                             m_update_player();

        void                             m_update_gui_module_inputs();
        void                             m_create_internal_timers();

        // TODO Maybe move this away from State class
        void m_render_bloom();

    

        UniformBuffer m_lights_ubo;
        std::array<Light, MAX_LIGHTS> m_lights;
        std::array<Light*, MAX_LIGHTS> m_light_ptrs { NULL };
        size_t m_num_lights { 0 };

        int64_t                                 m_focused_gui_module_idx { -1 };
        std::vector<std::unique_ptr<GuiModule>> m_gui_modules;
        std::map<int/*light ID*/, Light>        m_lights_pframe_map; // Previous frame lights.
       


    };

};




#endif
