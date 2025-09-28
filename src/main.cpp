
#include "ambient3d/ambient3d.hpp"
#include "ambient3d/animation.hpp"

#include <cstdio>
#include <raymath.h>
#include <chrono>


struct GameState {
    AM::Renderable tree;
    AM::Renderable robot;

    Material chunk_material;


    AM::Light** lightA { NULL };
    AM::Light** lightB { NULL };
    AM::Light** lightC { NULL };
    AM::Light** lightD { NULL };
};



void render_scene(AM::State* st, GameState* gst) {
    const float frame_time = GetFrameTime();
    
    //DrawSphere((*gst->lightA)->pos, 1.0f, (*gst->lightA)->color);
    DrawSphere((*gst->lightB)->pos, 1.0f, (*gst->lightB)->color);
    DrawSphere((*gst->lightC)->pos, 1.0f, (*gst->lightC)->color);
    DrawSphere((*gst->lightD)->pos, 1.0f, (*gst->lightD)->color);

    gst->tree.render();
 

    // Render other players in the server.
    st->net->foreach_online_players(
    [gst, frame_time](AM::N_Player* player) {
     
        gst->robot.anim.update(player->anim_id, frame_time, gst->robot.get_model());

        Matrix translation = MatrixTranslate(player->pos.x, player->pos.y, player->pos.z);
        Matrix body_rotation = MatrixRotateY(player->cam_yaw);
       
        // Head rotation.
        gst->robot.mesh_transforms[1] = MatrixRotateX(-player->cam_pitch);
        
        *gst->robot.transform = MatrixMultiply(body_rotation, translation);
        gst->robot.render();         
    });
}
 


void main_loop(AM::State* st) {

    GameState gst;


    gst.chunk_material = LoadMaterialDefault();
    gst.chunk_material.shader = st->shaders[AM::ShaderIDX::DEFAULT];

    gst.tree.load("test_models/tree.glb", { st->shaders[AM::ShaderIDX::DEFAULT] });
    gst.tree.mesh_attribute(1, AM::MeshAttrib{ .affected_by_wind = true });
    
    //gst.gun.load("test_models/m4.glb", { st->shaders[AM::ShaderIDX::DEFAULT] });
    //*gst.gun.transform = MatrixTranslate(7, 2, 20);

    gst.robot.load("test_models/robot.glb",
            { st->shaders[AM::ShaderIDX::DEFAULT] },
            AM::RLF_MESH_TRANSFORMS | AM::RLF_ANIMATIONS );

    gst.robot.anim.set_animation_speed(0, 0.025f);
    gst.robot.anim.set_animation_speed(1, 0.006f);

    // Valot -----------------------------------
    gst.lightA = st->add_light(AM::Light {
            .pos = Vector3(30, 10, -10),
            .color = Color(250, 230, 200, 255),
            .cutoff = 1.0f, .strength = 50.0f });

    gst.lightB = st->add_light(AM::Light {
            .pos = Vector3(-20, 10, -10),
            .color = GREEN,
            .cutoff = 5.0f, .strength = 13.0f });

    gst.lightC = st->add_light(AM::Light {
            .pos = Vector3(5, 3, 25),
            .color = Color(255, 50, 230, 255),
            .cutoff = 1.0f, .strength = 10.0f });

    gst.lightD = st->add_light(AM::Light {
            .pos = Vector3(5, 3, 10),
            .color = Color(50, 150, 230, 255),
            .cutoff = 1.0f, .strength = 10.0f });
    //--------------------------------------------

    AM::Chatbox* chatbox = st->find_gui_module<AM::Chatbox>(AM::GuiModuleID::CHATBOX);



    while(!WindowShouldClose()) {

        // TODO: Move these to engine side.
        AM::set_uniform_vec3(st->shaders[AM::ShaderIDX::DEFAULT].id, "u_view_pos", st->player.position());
        AM::set_uniform_float(st->shaders[AM::ShaderIDX::DEFAULT].id, "u_time", GetTime());
        AM::set_uniform_vec3(st->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_view_pos", st->player.position());
        AM::set_uniform_float(st->shaders[AM::ShaderIDX::DEFAULT_INSTANCED].id, "u_time", GetTime());
      
        if(IsKeyPressed(KEY_TAB)) {
            st->set_gui_module_focus(AM::GuiModuleID::CHATBOX, AM::GuiModuleFocus::TOGGLE);
            st->set_mouse_enabled(!st->is_mouse_enabled());
            st->set_movement_enabled(!st->is_movement_enabled());
        }
        
        if(IsKeyPressed(KEY_ENTER)) {
            if(chatbox->has_focus && !chatbox->text_input.empty()) {
                st->net->packet.prepare(AM::PacketID::CHAT_MESSAGE);
                st->net->packet.write<std::string>({ chatbox->text_input });
                st->net->send_packet(AM::NetProto::TCP);
                /*
                AM::packet_prepare(&st->net->packet, AM::PacketID::CHAT_MESSAGE);
                AM::packet_write_string(&st->net->packet, chatbox->text_input);
                st->net->send_packet(AM::NetProto::TCP);
                */
                chatbox->text_input.clear();
            }
        }

        if(IsKeyPressed(KEY_K)) {
    
            std::thread test_th_1([st](){
                printf("TEST_TRHEAD 1  Preparing packet.\n");
                st->net->packet.prepare(AM::PacketID::CHAT_MESSAGE);
                st->net->packet.write<int>({  123, 23, 2378, 283, 23, 232, 3,2, 3,3,  });
                st->net->send_packet(AM::NetProto::UDP);
            });

            std::thread test_th_2([st](){
                printf("TEST_TRHEAD 2  Preparing packet.\n");
                st->net->packet.prepare(AM::PacketID::CHAT_MESSAGE);
                printf("TEST_THREAD 1  Finished writing.\n");
                st->net->packet.write<int>({  123, 23 });
                st->net->send_packet(AM::NetProto::UDP);
            });


            test_th_1.join();
            test_th_2.join();

        }

        (*gst.lightA)->pos = st->player.position();

        (*gst.lightB)->pos.x += sin(GetTime()) * 0.01;
        (*gst.lightB)->pos.z += cos(GetTime()) * 0.01;
        (*gst.lightB)->pos.y += sin(GetTime() * 2.0) * 0.005;

        (*gst.lightC)->pos.x += cos(GetTime())*0.005;
        


        // Rendering...
        st->frame_begin();
        
        render_scene(st, &gst);  
        
        st->frame_end();
    }

    printf("Stopping...\n");
  
    gst.robot.unload();
    gst.tree.unload();
}


int main(int argc, char** argv) {

    AM::State st(1000, 800, 
            "Ambient3D - Development",
            "client_config.json",
            AM::NetConnectCFG { 
                .host      = "127.0.0.1",
                .tcp_port  = "34482",
                .udp_port  = "34485"
            });

    if(st.ready) {
        main_loop(&st);
    }
   
    return 0;
}

