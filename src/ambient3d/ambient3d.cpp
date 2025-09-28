#include "ambient3d.hpp"
#include "internal_shaders.hpp"
#include "util.hpp"


AM::State::State(
        uint16_t win_width,
        uint16_t win_height, 
        const char* title,
        const char* config_file,
        AM::NetConnectCFG network_cfg) {
 
    this->config = AM::Config(config_file);

    // Initialize network.

    this->register_gui_module<AM::Chatbox>(GuiModuleID::CHATBOX, AM::GuiModule::RenderOPT::ALWAYS);
    AM::Chatbox* chatbox = this->find_gui_module<AM::Chatbox>(GuiModuleID::CHATBOX);

    network_cfg.msg_recv_callback 
        = [chatbox](uint8_t r, uint8_t g, uint8_t b, const std::string& str)
        { chatbox->push_message(r, g, b, str); };

    this->net = new AM::Network(m_asio_io_context, network_cfg);
    this->net->set_engine_state(this);

    if(!this->net->is_connected()) {
        fprintf(stderr, "\033[31mFailed to connect to server (%s, ports: (tcp = %s, udp = %s))\033[0m\n", 
                network_cfg.host,
                network_cfg.tcp_port,
                network_cfg.udp_port);
        return;
    }

    printf("Waiting for complete connection...\n");
    while(!this->net->is_fully_connected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    // Initialize raylib.
   
    InitWindow(win_width, win_height, title);
    SetExitKey(KEY_NULL);

    SetTargetFPS(1000);
    DisableCursor();

    this->font = LoadFontEx((this->config.fonts_directory + this->config.font_file).c_str(), 64, 0, 0);
    SetTextureFilter(this->font.texture, TEXTURE_FILTER_BILINEAR);



    GLSL_preproc_add_meminclude("GLSL_VERSION", "#version 430\n");
    GLSL_preproc_add_meminclude("AMBIENT3D_LIGHTS", I_Shaders::LIGHTS_GLSL);

    SetTraceLogLevel(LOG_ALL);


    // TODO: Adding shaders to the vector like this
    // may be not ideal, bugs may happen where the index is not matching.

    // DEFAULT
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX).c_str(),
                GLSL_preproc(I_Shaders::DEFAULT_FRAGMENT).c_str()
                ));

    // DEFAULT_INSTANCED
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX, PREPROC_FLAGS::DEFINE__RENDER_INSTANCED).c_str(),
                GLSL_preproc(I_Shaders::DEFAULT_FRAGMENT).c_str()
                ));
    AM::init_instanced_shader(&this->shaders.back());
    

    // POST-PROCESSING
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX).c_str(),
                GLSL_preproc(I_Shaders::POSTPROCESS_FRAGMENT).c_str()
                ));
 
    // BLOOM_TRESHOLD
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX).c_str(),
                GLSL_preproc(I_Shaders::BLOOM_TRESH_FRAGMENT).c_str()
                ));  

    // BLOOM_DOWNSAMPLE_FRAGMENT
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX).c_str(),
                GLSL_preproc(I_Shaders::BLOOM_DOWNSAMPLE_FRAGMENT).c_str()
                ));  

    // BLOOM_UPSAMPLE_FRAGMENT
    this->add_shader(LoadShaderFromMemory(
                GLSL_preproc(I_Shaders::DEFAULT_VERTEX).c_str(),
                GLSL_preproc(I_Shaders::BLOOM_UPSAMPLE_FRAGMENT).c_str()
                ));  


    this->item_manager.set_item_default_shader(this->shaders[AM::ShaderIDX::DEFAULT]);
    SetTraceLogLevel(LOG_NONE);


    // Create rendering targets.
    // TODO: Move to separate function.-

    m_render_targets[RenderTargetIDX::RESULT]
        = LoadRenderTexture(win_width, win_height);

    m_render_targets[RenderTargetIDX::BLOOM_TRESHOLD]
        = LoadRenderTexture(win_width, win_height);
    
    m_render_targets[RenderTargetIDX::BLOOM_PRE_RESULT]
        = LoadRenderTexture(win_width, win_height);
    
    m_render_targets[RenderTargetIDX::BLOOM_RESULT]
        = LoadRenderTexture(win_width, win_height);



    // Create bloom samples.
    // TODO: Move to separate function-
    
    int sample_res_X = win_width;
    int sample_res_Y = win_height;
    constexpr float scale_factor = 0.85f;


    for(size_t i = 0; i < m_bloom_samples.size(); i++) {
        
        m_bloom_samples[i] = LoadRenderTexture(sample_res_X, sample_res_Y);
        SetTextureFilter(m_bloom_samples[i].texture, TEXTURE_FILTER_BILINEAR);
        sample_res_X = round((float)sample_res_X * scale_factor);
        sample_res_Y = round((float)sample_res_Y * scale_factor);

    }
    


    SetTraceLogLevel(LOG_ALL);

    // TODO: Create memory include for light glsl (for number of lights)

    m_lights_ubo.create(1, { 
            UBO_ELEMENT {
                .num = 64, .elem_sizeb = 48
            },
            UBO_ELEMENT {
                .num = 1, .elem_sizeb = 4
            }});

   
    this->player.set_engine_state(this);
    this->terrain.set_engine_state(this);
    this->terrain.create_chunk_materials();
 
    m_create_internal_timers();
    
    this->ready = true;
}

AM::State::~State() {
    if(this->net) {
        if(this->net->is_connected()) {
            this->net->close(m_asio_io_context);
        }
            
        delete this->net;
    }

    if(!this->ready) {
        return;
    }

    m_lights_ubo.free();

    SetTraceLogLevel(LOG_NONE);
    // Unload render targets.
    for(size_t i = 0; i < m_render_targets.size(); i++) {
        UnloadRenderTexture(m_render_targets[i]);
    }
    for(size_t i = 0; i < m_bloom_samples.size(); i++) {
        UnloadRenderTexture(m_bloom_samples[i]);
    }
    SetTraceLogLevel(LOG_ALL);

    this->terrain.free_regenbuf();
    this->terrain.unload_all_chunks();
    this->terrain.unload_materials();

    for(Shader& shader : this->shaders) {
        UnloadShader(&shader);
    }

    UnloadFont(this->font);
    CloseWindow();
}
        
void AM::State::m_create_internal_timers() {

    this->create_named_timer("FAST_FIXED_TICK_TIMER");
    this->create_named_timer("SLOW_FIXED_TICK_TIMER");

    this->create_named_timer("PLAYER_YPOS_INTERP_TIMER");
    
    // Create callback to reset PLAYER_YPOS_INTERP_TIMER
    this->net->add_packet_callback(AM::NetProto::UDP, AM::PacketID::PLAYER_POSITION,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)data; (void)sizeb;
        
        AM::Timer* timer = this->get_named_timer("PLAYER_YPOS_INTERP_TIMER");
        timer->reset();
    });
}

void AM::State::set_mouse_enabled(bool enabled) {
    m_mouse_enabled = enabled;
    if(enabled) {
        DisableCursor();
    }
    else {
        EnableCursor();
    }
}

void AM::State::add_shader(const Shader& shader) {
    if(!IsShaderValid(shader)) {
        fprintf(stderr, "ERROR! Cant add broken shader to state.\n");
        return;
    }
    this->shaders.push_back(shader);
}

AM::Light** AM::State::add_light(const Light& light) {
    if(m_num_lights+1 >= AM::MAX_LIGHTS) {
        fprintf(stderr, "Increase the light array size or remove unused lights.\n");
        return NULL;
    }

    m_lights[m_num_lights] = light;
    m_light_ptrs[m_num_lights] = &m_lights[m_num_lights];
    Light** result = &m_light_ptrs[m_num_lights];

    m_num_lights++;
    // Update lights uniform buffer "num_lights"
    m_lights_ubo.update_element(m_lights_ubo.size()-1, (void*)&m_num_lights, sizeof(int));

    return result;
}

void AM::State::remove_light(Light** light) {
    if(!light) { return; }
    if(!(*light)) { return; }

    size_t id = (*light)->id;

    for(uint64_t i = id; i < m_num_lights-1; i++) {
        m_lights[i] = m_lights[i+1];
        if(id > 0) {
            m_lights[i].id--;
        }
        m_lights[i].force_update = true;
    }
    m_num_lights--;

    // Update pointers.
    for(size_t i = 0; i < m_num_lights; i++) {
        m_light_ptrs[i] = &m_lights[i];
    }

    *light = NULL;
    // Update lights uniform buffer "num_lights"
    m_lights_ubo.update_element(m_lights_ubo.size()-1, (void*)&m_num_lights, sizeof(int));
}


// Updates lights only if they have changed even little bit.
void AM::State::update_lights() {
    for(size_t i = 0; i < m_lights.size(); i++) {
        Light& light = m_lights[i];

        light.id = i;
        bool need_update = false;
        if(light.force_update) {
            m_lights_pframe_map[light.id] = light;
            light.force_update = false;
            need_update = true;
        }
        else {
            const auto search = m_lights_pframe_map.find(light.id);
            if(search == m_lights_pframe_map.end()) {
                need_update = true;
                m_lights_pframe_map.insert(std::make_pair(light.id, m_lights[i]));
            }
            else {
                if((need_update = !light.equal(search->second))) {
                    m_lights_pframe_map[light.id] = light;       
                }
            }
        }

        if(need_update) {
            float light_data[] = {
                light.pos.x,
                light.pos.y,
                light.pos.z,
                0.0f,

                (float)light.color.r / 255.0f,
                (float)light.color.g / 255.0f,
                (float)light.color.b / 255.0f,
                1.0f,

                light.strength,
                light.cutoff,
                0.0f,
                0.0f
            };

            m_lights_ubo.update_element(light.id, light_data, sizeof(light_data));
        }
    }
}

void AM::State::set_vision_effect(float amount) {
    AMutil::clamp<float>(amount, 0.0f, 1.0f);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::POST_PROCESSING].id, "u_vision_effect", amount);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::POST_PROCESSING].id, "u_time", GetTime());
}

void AM::State::draw_text(int font_size, const char* text, int x, int y, const Color& color) {
    DrawTextEx(this->font, text, Vector2(x, y), font_size, 1.0f, color);
}


void AM::State::draw_info() {
    int text_y = 10;
    int text_x = 15;
    constexpr int y_add = 16;
    constexpr int font_size = 18;
    draw_text(font_size, TextFormat("FPS %i", GetFPS()), text_x, text_y, WHITE);
    text_y += y_add;
    const Vector3 player_pos = this->player.position();
    draw_text(font_size, TextFormat("XYZ = (%0.2f, %0.2f, %0.2f)", 
                player_pos.x,
                player_pos.y,
                player_pos.z
                ), text_x, text_y, WHITE);
    text_y += y_add;
    draw_text(font_size, TextFormat("OnGround = %s", this->player.on_ground ? "Yes" : "No"),
            text_x, text_y, WHITE);
    text_y += y_add;
    draw_text(font_size, TextFormat("MovmentEnabled = %s", m_movement_enabled ? "Yes" : "No"),
            text_x, text_y, WHITE);
    text_y += y_add;
    draw_text(font_size, TextFormat("MouseEnabled = %s", m_mouse_enabled ? "Yes" : "No"),
            text_x, text_y, WHITE);

}


void AM::State::m_render_dropped_items() {
    
    auto items = this->item_manager.get_dropped_items();

    for(auto it = items->begin(); it != items->end(); ++it) {
        const AM::Item* item = &it->second;
       
        if(item->renderable.get() == NULL) {
            continue;
        }
        if(!item->renderable->is_loaded()) {
            continue;
        }

        *item->renderable->transform = MatrixTranslate(item->pos_x, item->pos_y, item->pos_z);
        item->renderable->render();
    }
}
        
void AM::State::frame_begin() {
    BeginTextureMode(m_render_targets[RenderTargetIDX::RESULT]);
    ClearBackground(BLACK);
    BeginMode3D(this->player.camera);

    m_slow_fixed_tick_update();
    m_fast_fixed_tick_update();
    m_update_gui_module_inputs();
    this->player.update_position_from_server();
    
    m_render_dropped_items();

    // TODO: Move these.
    //       User may need better control.
    if(m_mouse_enabled) {
        this->player.update_camera();
    }
    this->player.update_movement(m_movement_enabled);
    this->player.update_animation();

    this->update_lights();
    this->terrain.render();
}
            
void AM::State::m_fast_fixed_tick_update() {
    AM::Timer* fast_tick_timer = this->get_named_timer("FAST_FIXED_TICK_TIMER");
    if(fast_tick_timer->time_sc() >= AM::FAST_FIXED_TICK_DELAY_SECONDS) {
        fast_tick_timer->reset();

        const Vector3 player_pos = this->player.position();

        // Update all items the server has sent.
        this->item_manager.update_items_queue();
        this->item_manager.cleanup_unused_items(player_pos);

        this->terrain.update_chunkdata_queue();

        this->net->packet.prepare(AM::PacketID::PLAYER_MOVEMENT_AND_CAMERA);
        this->net->packet.write<int>({ this->net->player_id, this->player.animation_id() });
        this->net->packet.write<float>({
                player_pos.x,
                player_pos.y,
                player_pos.z,
                this->player.camera_yaw(),
                this->player.camera_pitch()
        });
        this->net->send_packet(AM::NetProto::UDP);

        // Callback to user if needed.
        if(m_fast_fixed_tick_callback) {
            m_fast_fixed_tick_callback(this);
        }
    }
}

void AM::State::m_slow_fixed_tick_update() {
    AM::Timer* slow_tick_timer = this->get_named_timer("SLOW_FIXED_TICK_TIMER");
    if(slow_tick_timer->time_sc() >= AM::SLOW_FIXED_TICK_DELAY_SECONDS) {
        slow_tick_timer->reset();

        // Unload far away chunks and send updates to the server.
        // This should be optimized a bit in the future 
        // but doesnt matter right now because this code runs only about every 6 seconds.

        printf("SLOW TICK!\n");
        std::vector<AM::ChunkPos> unloaded_chunk_positions;
        AM::ChunkPos player_chunk_pos = this->player.chunk_pos();

        printf("%i, %i\n", player_chunk_pos.x, player_chunk_pos.z);

        for(auto chunk_it = this->terrain.chunk_map.begin();
                chunk_it != this->terrain.chunk_map.end(); ) {
            AM::Chunk* chunk = &chunk_it->second;
            
            if(chunk->pos.distance(player_chunk_pos) > this->config.render_distance) {
                chunk->unload();
            }

            if(!chunk->is_loaded()) {
                unloaded_chunk_positions.push_back(chunk->pos);
                this->terrain.chunk_map.erase(chunk_it++);
                continue;
            }

            ++chunk_it;
        }

        printf("Num chunks unloaded: %li\n", unloaded_chunk_positions.size());

        if(!unloaded_chunk_positions.empty()) {
            
            //AM::packet_prepare(&this->net->packet, );
        }

        // Callback to user if needed.
        if(m_slow_fixed_tick_callback) {
            m_slow_fixed_tick_callback(this);
        }
    }
}

void AM::State::m_update_gui_module_inputs() {
    if(m_focused_gui_module_idx < 0) {
        return;
    }

    auto& module_ = m_gui_modules[m_focused_gui_module_idx];
    if(!module_->has_focus) {
        m_focused_gui_module_idx = -1;
        return;
    }

    module_->module__char_input(GetCharPressed());
}


void AM::State::m_render_bloom() {

    // Get bloom treshold texture.
    AMutil::resample_texture(
            this,
            /* TO */   m_bloom_samples[0],
            /* FROM */ m_render_targets[RenderTargetIDX::RESULT],
            ShaderIDX::BLOOM_TRESHOLD
            );

    size_t p = 0;

    // Scale down and filter the texture each step.
    for(size_t i = 1; i < m_bloom_samples.size(); i++) {
        p = i - 1; // Previous sample.

        AMutil::resample_texture(
                this,
                /* TO */   m_bloom_samples[i],
                /* FROM */ m_bloom_samples[p],
                ShaderIDX::BLOOM_DOWNSAMPLE_FILTER
                );
    }

    // Scale up and filter the texture each step.
    for(size_t i = m_bloom_samples.size()-2; i > 0; i--) {
        p = i + 1; // Previous sample.

        AMutil::resample_texture(
                this,
                /* TO */   m_bloom_samples[i],
                /* FROM */ m_bloom_samples[p],
                ShaderIDX::BLOOM_UPSAMPLE_FILTER
                );
    }


    // Now the result is ready.

    AMutil::resample_texture(
            this,
            /* TO */   m_render_targets[RenderTargetIDX::BLOOM_RESULT],
            /* FROM */ m_bloom_samples[1],
            -1
            );
}

/*
static void _draw_tex(const Texture2D& tex, int X, int Y, float scale, bool invert) {
    DrawTexturePro(tex,
            (Rectangle){
                0, 0, (float)tex.width, ((invert) ? -1.0 : 1.0) * (float)(tex.height)
            },
            (Rectangle){
                0, 0, (float)tex.width * scale, ((invert) ? -1.0 : 1.0) * (float)(tex.height * scale)
            },
            (Vector2){ -X, Y }, 0, WHITE);
}
*/
void AM::State::frame_end() {
    EndMode3D();
    EndTextureMode();


    m_render_bloom();


    // Postprocessing.

    BeginDrawing();
    
    ClearBackground(BLACK);
    const Shader& shader = this->shaders[ShaderIDX::POST_PROCESSING];
    BeginShaderMode(shader);
    
    AM::set_uniform_sampler(shader.id, "texture_bloom", m_render_targets[RenderTargetIDX::BLOOM_RESULT].texture, 3);
    //AM::set_uniform_sampler(shader.id, "texture_bloom", m_bloom_samples[1].texture, 3);

    const int width  = m_render_targets[RenderTargetIDX::RESULT].texture.width;
    const int height = m_render_targets[RenderTargetIDX::RESULT].texture.height;
    DrawTextureRec(m_render_targets[RenderTargetIDX::RESULT].texture,
            (Rectangle){
                0, 0, (float)width, (float)-height
            },
            (Vector2){ 0, 0 }, WHITE);

    EndShaderMode();


    // Render GuiModules.
    for(size_t i = 0; i < m_gui_modules.size(); i++) {
        switch(m_gui_modules[i]->get_render_option()) {
            case GuiModule::RenderOPT::WHEN_FOCUSED:
                if(m_gui_modules[i]->has_focus) {
                    m_gui_modules[i]->module__render(&this->font);
                }
                break;
            case GuiModule::RenderOPT::ALWAYS:
                m_gui_modules[i]->module__render(&this->font);
                break;
        }
    }

    this->draw_info();

    EndDrawing();

    // Update timers.
    float frame_time = GetFrameTime();
    for(auto timer_it = m_named_timers.begin(); timer_it != m_named_timers.end(); ++timer_it) {
        AM::Timer* timer = timer_it->second.get();
        if(timer->was_reset_this_frame()) {
            timer->clear_reset_flag();
            continue;
        }
        timer->update(frame_time);
    }
}

 
void AM::State::set_gui_module_focus(int module_id, AM::GuiModuleFocus focus_option) {
    m_focused_gui_module_idx = -1;

    for(size_t i = 0; i < m_gui_modules.size(); i++) {
        
        if(module_id == m_gui_modules[i]->get_id()) {
            switch(focus_option) {
                case GuiModuleFocus::GAIN:
                    m_gui_modules[i]->has_focus = true;
                    break;
                case GuiModuleFocus::LOSE:
                    m_gui_modules[i]->has_focus = true;
                    break;
                case GuiModuleFocus::TOGGLE:
                    m_gui_modules[i]->has_focus = !m_gui_modules[i]->has_focus;
                    break;
            }
            m_focused_gui_module_idx = i;
        }
        else {
            m_gui_modules[i]->has_focus = false;
        }
        
    }
}


void AM::State::create_named_timer(const std::string_view& timer_name) {
    std::lock_guard<std::mutex> lock(m_named_timers_mutex);

    auto timer_search = m_named_timers.find(timer_name);
    if(timer_search != m_named_timers.end()) {
        fprintf(stderr, "ERROR! %s: \"%s\" Already exists. Cant overwrite.\n",
                __func__, std::string(timer_name).c_str());
        return;
    }

    m_named_timers.insert(std::make_pair(timer_name, std::make_shared<AM::Timer>()));
}

void AM::State::delete_named_timer(const std::string_view& timer_name) {
    std::lock_guard<std::mutex> lock(m_named_timers_mutex);
    
    auto timer_search = m_named_timers.find(timer_name);
    if(timer_search == m_named_timers.end()) {
        fprintf(stderr, "ERROR! %s: \"%s\" Doesnt exist.\n",
                __func__, std::string(timer_name).c_str());
        return;
    }

    m_named_timers.erase(timer_search);
}

AM::Timer* AM::State::get_named_timer(const std::string_view& timer_name) {
    std::lock_guard<std::mutex> lock(m_named_timers_mutex);
    
    auto timer_search = m_named_timers.find(timer_name);
    if(timer_search == m_named_timers.end()) {
        fprintf(stderr, "ERROR! %s: \"%s\" Doesnt exist.\n",
                __func__, std::string(timer_name).c_str());
        return NULL;
    }

    return timer_search->second.get();
}

