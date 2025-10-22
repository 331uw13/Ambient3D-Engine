#include "ambient3d.hpp"
#include "internal_shaders.hpp"
#include "util.hpp"
#include "rlgl.h"
#include "bloom.hpp"


AM::State::State(
        uint16_t win_width,
        uint16_t win_height, 
        const char* title,
        const AM::ClientConfig& config,
        AM::NetConnectCFG network_cfg) {
 
    this->config = config;//AM::ClientConfig(config_file);

    // Initialize network.

    /*network_cfg.msg_recv_callback 
        = [chatbox](uint8_t r, uint8_t g, uint8_t b, const std::string& str)
        { chatbox->push_message(r, g, b, str); };
    */

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



    GLSL_preproc_add_meminclude("GLSL_VERSION", AM::ShaderCode::get(AM::ShaderCode::GLSL_VERSION));
    GLSL_preproc_add_meminclude("AMBIENT3D_LIGHTS", AM::ShaderCode::get(AM::ShaderCode::LIGHTS_GLSL));
    GLSL_preproc_add_meminclude("AMBIENT3D_FOG", AM::ShaderCode::get(AM::ShaderCode::FOG_GLSL));

    SetTraceLogLevel(LOG_ALL);


    // TODO: Move shader loading somewhere else.

    // DEFAULT
    this->set_shader(AM::ShaderIDX::DEFAULT,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_FRAGMENT)).c_str()
                ));

    // DEFAULT_INSTANCED
    this->set_shader(AM::ShaderIDX::DEFAULT_INSTANCED,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX), 
                        PREPROC_FLAGS::DEFINE__RENDER_INSTANCED).c_str(),

                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_FRAGMENT)).c_str()
                ));

    AM::init_instanced_shader(&this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED]);
    
    // POST_PROCESSING
    this->set_shader(AM::ShaderIDX::POST_PROCESSING,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::POSTPROCESS_FRAGMENT)).c_str()
                ));
 
    // BLOOM_TRESHOLD
    this->set_shader(AM::ShaderIDX::BLOOM_TRESHOLD,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::BLOOM_TRESH_FRAGMENT)).c_str()
                ));  

    // BLOOM_DOWNSAMPLE_FRAGMENT
    this->set_shader(AM::ShaderIDX::BLOOM_DOWNSAMPLE_FILTER,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::BLOOM_DOWNSAMPLE_FRAGMENT)).c_str()
                ));  

    // BLOOM_UPSAMPLE_FRAGMENT
    this->set_shader(AM::ShaderIDX::BLOOM_UPSAMPLE_FILTER,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::BLOOM_UPSAMPLE_FRAGMENT)).c_str()
                ));  

    // SKYBOX_FRAGMENT
    this->set_shader(AM::ShaderIDX::SKYBOX,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::SKYBOX_FRAGMENT)).c_str()
                ));

    // SINGLECOLOR_FRAGMENT
    this->set_shader(AM::ShaderIDX::SINGLE_COLOR,
            LoadShaderFromMemory(
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::DEFAULT_VERTEX)).c_str(),
                GLSL_preproc(AM::ShaderCode::get(AM::ShaderCode::SINGLECOLOR_FRAGMENT)).c_str()
                ));


    this->item_manager.set_item_default_shader(this->shaders[AM::ShaderIDX::DEFAULT]);
    SetTraceLogLevel(LOG_NONE);

    // Create skybox.
    m_skybox_model = LoadModelFromMesh(GenMeshSphere(1.0f, 8, 8));
    m_skybox_model.materials[0].shader = this->shaders[AM::ShaderIDX::SKYBOX];


    // Create rendering targets.
    // TODO: Move to separate function.-

    m_render_targets[RenderTargetIDX::RESULT]
        = LoadRenderTexture(win_width, win_height);

    m_render_targets[RenderTargetIDX::BLOOM_TRESHOLD]
        = LoadRenderTexture(win_width, win_height);
    
    m_render_targets[RenderTargetIDX::BLOOM_RESULT]
        = LoadRenderTexture(win_width, win_height);
   
    /*
    m_render_targets[RenderTargetIDX::BLOOM_PRE_RESULT]
        = LoadRenderTexture(win_width, win_height);
    */


    constexpr int bloom_sample_targets = 16;
    constexpr float bloom_scale_factor = 0.85f;
    AM::Bloom::create_sample_targets(bloom_sample_targets, bloom_scale_factor);



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
    this->item_manager.set_engine_state(this);
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

    AM::Bloom::unload_sample_targets();

    SetTraceLogLevel(LOG_ALL);

    this->terrain.free_regenbuf();
    this->terrain.unload_all_chunks();
    this->terrain.unload_materials();

    for(size_t i = 0; i < AM::ShaderIDX::NUM_SHADERS; i++) {
        UnloadShader(&this->shaders[i]);
    }

    UnloadFont(this->font);
    CloseWindow();
}
        
void AM::State::m_create_internal_timers() {

    this->create_named_timer("FAST_FIXED_TICK_TIMER");
    this->create_named_timer("SLOW_FIXED_TICK_TIMER");

    this->create_named_timer("PLAYER_YPOS_INTERP_TIMER");
    //this->create_named_timer("TIMEOFDAY_INTERP_TIMER");

    // Create callback to reset PLAYER_YPOS_INTERP_TIMER
    this->net->add_packet_callback(AM::NetProto::UDP, AM::PacketID::PLAYER_POSITION,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)data; (void)sizeb; (void)interval_ms;
        
        AM::Timer* timer = this->get_named_timer("PLAYER_YPOS_INTERP_TIMER");
        timer->reset();
    });

    /*
    // Create callback to reset TIMEOFDAY_INTERP_TIMER
    this->net->add_packet_callback(AM::NetProto::UDP, AM::PacketID::TIME_OF_DAY,
    [this](float interval_ms, char* data, size_t sizeb) {
        (void)data; (void)sizeb; (void)interval_ms;
        
        AM::Timer* timer = this->get_named_timer("TIMEOFDAY_INTERP_TIMER");
        timer->reset();
    });
    */


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

void AM::State::set_shader(int shader_idx, const Shader& shader) {
    if(!IsShaderValid(shader)) {
        fprintf(stderr, "ERROR! %s: Cant add broken shader.\n", __func__);
        return;
    }

    this->shaders[shader_idx] = shader;
}

void AM::State::set_material(int material_idx, const Material& material) {
    if(!IsMaterialValid(material)) {
        fprintf(stderr, "ERROR! %s: Cant add broken material.\n", __func__);
        return;
    }

    this->materials[material_idx] = material;
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
    text_y += y_add;
    draw_text(font_size, TextFormat("TimeOfDay: (raw=%f, curve=%f)", this->raw_timeofday, this->timeofday_curve),
            text_x, text_y, WHITE);
    text_y += y_add;

}

void AM::State::m_update_dropped_items() {    
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

        if(!(m_flags & AM::StateFlags::DONT_RENDER_DEFAULT_ITEMINFO)) {
            m_render_default_iteminfo(item);
        }

        if(IsKeyPressed(KEY_E)) {
            float distance = Vector3Distance(
                    Vector3(item->pos_x, item->pos_y, item->pos_z),
                    this->player.position());
            if(distance < this->net->server_cfg.item_pickup_distance) {
                this->net->packet.prepare(AM::PacketID::PLAYER_PICKUP_ITEM);
                this->net->packet.write<int>({ item->uuid });
                this->net->send_packet(AM::NetProto::TCP);
            }
        }
    }
}
            
void AM::State::m_render_default_iteminfo(const AM::Item* item) {
    Vector3 item_pos = Vector3(item->pos_x, item->pos_y, item->pos_z);
    Vector3 player_pos = this->player.position();
    float distance = Vector3Distance(item_pos, player_pos);
    if(distance >= this->net->server_cfg.item_pickup_distance) {
        return;
    }

    Vector2 towards_player = AMutil::get_rotation_towards(item_pos, player_pos);
    towards_player *= RAD2DEG;

    float normalized_dist = 1.0 - (distance / this->net->server_cfg.item_pickup_distance);
    normalized_dist = std::clamp(normalized_dist*2.0f, 0.0f, 1.0f);

    Color text_color = Color(255, 255, 255, (unsigned char)(normalized_dist * 255.0f));

    this->draw_text_3D(item->display_name,
            0.03f, // Font scale.
            1.0f,  // Font spacing.
            text_color,
            Vector2(40.0f, 0.0f), // Origin xy.
            item_pos,
            Vector3(towards_player.x, towards_player.y, 0.0f));

    text_color.a *= 0.5f;

    this->draw_text_3D("[E] to pickup",
            0.016f, // Font scale.
            1.0f,   // Font spacing.
            text_color,
            Vector2(80.0f, 20.0f), // Origin xy.
            item_pos,
            Vector3(towards_player.x, towards_player.y, 0.0f));
}

void AM::State::draw_text_3D(const char* text,
        float font_scale,
        float font_spacing,
        Color color,
        Vector2 originxy,
        Vector3 position,
        Vector3 rotation
){
    BeginShaderMode(this->shaders[AM::ShaderIDX::SINGLE_COLOR]);

    constexpr int font_size = 10;
    const float text_width = MeasureTextEx(this->font,text, font_size, font_spacing).x;

    rlPushMatrix();
    rlLoadIdentity();
    rlTranslatef(0, 0, 0);
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(180.0f, 0.0, 0.0, 1.0);
    rlRotatef(rotation.x, 0.0, 1.0, 0.0);
    rlRotatef(rotation.y, 1.0, 0.0, 0.0);
    rlRotatef(rotation.z, 0.0, 0.0, 1.0);
    rlScalef(font_scale, font_scale, font_scale);

    AM::set_uniform_vec4(this->shaders[AM::ShaderIDX::SINGLE_COLOR].id, "u_color", 
            Vector4(
                (float)color.r / 255.0f,
                (float)color.g / 255.0f,
                (float)color.b / 255.0f,
                (float)color.a / 255.0f));
    DrawTextEx(this->font, text, Vector2(-text_width/2, 0)+originxy, font_size, font_spacing, color);

    rlPopMatrix();
    EndShaderMode();
}


void AM::State::m_update_player() {
 
    this->player.set_terrain_surface_y(this->terrain.get_surface_level(this->player.position()));
    this->player.update_position_from_server();
    this->player.update_gravity();

}

void AM::State::m_set_shader_uniforms() {
    const Vector3 player_pos = this->player.position();
    const float time = GetTime();
    const float fog_density = this->net->dynamic_data.get_float(AM::NDD_ID::FOG_DENSITY);
    const Vector3 fog_color = this->net->dynamic_data.get_vector3(AM::NDD_ID::FOG_COLOR);

    // TODO: Use uniform buffer for data which may update every frame.

    AM::set_uniform_vec3(this->shaders[AM::ShaderIDX::DEFAULT].id, "u_view_pos", player_pos);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT].id, "u_timeofday_curve", this->timeofday_curve);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT].id, "u_time", time);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT].id, "u_fog_density", fog_density);
    AM::set_uniform_vec3(this->shaders[AM::ShaderIDX::DEFAULT].id, "u_fog_color", fog_color);

    AM::set_uniform_vec3(this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_view_pos", player_pos);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_timeofday_curve", this->timeofday_curve);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_time", time);
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_fog_density", fog_density);
    AM::set_uniform_vec3(this->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_fog_color", fog_color);
    
    AM::set_uniform_float(this->shaders[AM::ShaderIDX::SKYBOX].id, "u_timeofday_curve", this->timeofday_curve);
    AM::set_uniform_vec3(this->shaders[AM::ShaderIDX::SKYBOX].id, "u_fog_color", fog_color);
}
            
void AM::State::m_render_skybox() {
    const Vector3 player_pos = this->player.position();
    Vector3 fog_color = this->net->dynamic_data.get_vector3(AM::NDD_ID::FOG_COLOR);
   
    fog_color *= this->timeofday_curve;
    fog_color *= 0.5f;

    rlDisableDepthMask();
    rlDisableBackfaceCulling();

    DrawModel(m_skybox_model, player_pos, 50.0f, BLACK);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}


void AM::State::frame_begin() {
    m_set_shader_uniforms();

    BeginTextureMode(m_render_targets[RenderTargetIDX::RESULT]);
    ClearBackground(BLACK);
    BeginMode3D(this->player.camera);

    m_render_skybox();

    this->update_lights();
    this->terrain.render();

    m_slow_fixed_tick_update();
    m_fast_fixed_tick_update();
    m_update_player();
    m_update_dropped_items();

    // TODO: Move these.
    //       User may need better control.
    if(m_mouse_enabled) {
        this->player.update_camera();
    }
    this->player.update_movement(m_movement_enabled);
    //this->player.update_animation();

}

void AM::State::frame_end() {
    m_update_timeofday();
    
    EndMode3D();
    EndTextureMode();


    AM::Bloom::render(
            &m_render_targets[RenderTargetIDX::BLOOM_RESULT],
             m_render_targets[RenderTargetIDX::RESULT],
            &this->shaders[AM::ShaderIDX::BLOOM_TRESHOLD],
            &this->shaders[AM::ShaderIDX::BLOOM_UPSAMPLE_FILTER],
            &this->shaders[AM::ShaderIDX::BLOOM_DOWNSAMPLE_FILTER]
    );


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
        // but doesnt matter right now because this code runs only about every 4 seconds.

        std::vector<AM::ChunkPos> unloaded_chunk_positions;
        AM::ChunkPos player_chunk_pos = this->player.chunk_pos();

        int num_chunks = 0;

        for(auto chunk_it = this->terrain.chunk_map.begin();
                chunk_it != this->terrain.chunk_map.end(); ) {
            AM::Chunk* chunk = &chunk_it->second;
            num_chunks++;
            
            if(!this->terrain.chunkpos_in_renderdist(player_chunk_pos, chunk->pos)) {
                unloaded_chunk_positions.push_back(chunk->pos);
                chunk->unload();
                chunk_it = this->terrain.chunk_map.erase(chunk_it);
                continue;
            }

            ++chunk_it;
        }

        if(!unloaded_chunk_positions.empty()) {
            printf("[STATE]: Unloaded %li chunks.\n", unloaded_chunk_positions.size());
            
            this->net->packet.prepare(AM::PacketID::PLAYER_UNLOADED_CHUNKS);
            for(const AM::ChunkPos& chunk_pos : unloaded_chunk_positions) {
                this->net->packet.write<int>({ chunk_pos.x, chunk_pos.z });
            }
            this->net->send_packet(AM::NetProto::TCP);
        }

        // Callback to user if needed.
        if(m_slow_fixed_tick_callback) {
            m_slow_fixed_tick_callback(this);
        }
    }
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
            
void AM::State::m_update_timeofday() {
    if(this->net->needto_sync_timeofday) {
        printf("[STATE]: Time of day has been synchronized.\n");
        this->net->needto_sync_timeofday = false;
        this->raw_timeofday = this->net->dynamic_data.get_float(AM::NDD_ID::TIMEOFDAY_SYNC);
    }

    this->raw_timeofday += GetFrameTime() / (this->net->server_cfg.day_cycle_in_minutes * 60.0f);
    if(this->raw_timeofday >= 1.0f) {
        this->raw_timeofday = 0.0f;
    }
    this->timeofday_curve = (1.0f-cos(2.0f*(this->raw_timeofday*M_PI)))*0.5f;
    this->timeofday_curve = std::clamp(this->timeofday_curve, 0.0001f, 0.9999f);
}


