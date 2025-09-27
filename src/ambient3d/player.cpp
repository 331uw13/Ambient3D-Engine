#include <cstdio>


#include "player.hpp"
#include "util.hpp"
#include "ambient3d.hpp"

#include "raymath.h"

Vector3 AM::Player::position() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_position;
}

void AM::Player::set_position(const Vector3& pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_position = pos;
}

Vector3 AM::Player::velocity() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_velocity;
}

void AM::Player::set_velocity(const Vector3& vel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_velocity = vel;
}


float AM::Player::walking_speed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_walking_speed;
}


void AM::Player::set_walking_speed(float speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_walking_speed = speed;
}

float AM::Player::running_speed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_running_speed;
}

void AM::Player::set_running_speed(float speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running_speed = speed;
}

float AM::Player::movement_friction() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_movement_friction;
}

void AM::Player::set_movement_friction(float f) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_movement_friction = f;
}

Vector3 AM::Player::camera_forward() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_camera_forward;
}
            
Vector3 AM::Player::camera_right() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_camera_right;
}

float AM::Player::camera_yaw() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_camera_yaw;
}

void AM::Player::set_camera_yaw(float yaw) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_camera_yaw = yaw;
}

float AM::Player::camera_pitch() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_camera_pitch;
}

void AM::Player::set_camera_pitch(float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_camera_pitch = pitch;
}

            
AM::AnimID AM::Player::animation_id() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_animation_id;
}

void AM::Player::set_animation_id(AM::AnimID anim_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_animation_id = anim_id;
}

            
AM::ChunkPos AM::Player::chunk_pos() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chunk_pos;
}

void AM::Player::set_chunk_pos(const AM::ChunkPos& chunk_pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_chunk_pos = chunk_pos;
}

float AM::Player::camera_sensetivity() const {
    return m_camera_sensetivity;
}

void AM::Player::set_camera_sensetivity(float sensetivity) {
    m_camera_sensetivity = sensetivity;
}


AM::Player::Player() {
    this->camera.target = Vector3(0.1f, 0.0f, 0.0f);
    this->camera.up = AM::UP_VECTOR;
    this->camera.fovy = 70.0f;
    this->camera.projection = CAMERA_PERSPECTIVE;
    m_camera_sensetivity = 0.0028f;
    /*
    this->cam.position = Vector3(0.0f, 0.0f, 0.0f);
    this->cam.target = Vector3(0.1f, 0.0f, 0.0f);
    this->cam.up = Vector3(0.0f, 1.0f, 0.0f);
    this->cam.fovy = 70.0f;
    this->cam.projection = CAMERA_PERSPECTIVE;
    this->vel = Vector3(0.0f, 0.0f, 0.0f);
    this->pos = Vector3(0.0f, 10.0f, 1.0f);
    this->speed = 40.0f;
    this->noclip = { false };
    this->noclip_speed = 250.0f;
    this->cam_sensetivity = 0.0028f;
    this->height = 1.85f;
    this->on_ground = false;
    this->pos_prevframe = this->pos;
    */

    this->Y_pos_update_stack.set_size_limit(3);
    this->XZ_pos_update_stack.set_size_limit(3);
    this->prevframe_positions.set_size_limit(64);
}

AM::Player::~Player() {
}
 
        
void AM::Player::update_animation() {

    /*
    // IDLE is default.
    this->anim_id = AM::AnimID::IDLE;

    if(this->is_moving) {
        this->anim_id = AM::AnimID::WALKING;
    }
    if(this->is_running) {
        this->anim_id = AM::AnimID::RUNNING;
    }
    */
}


void AM::Player::update_movement(bool handle_user_input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    this->prevframe_positions.push_front(m_position);

    const float frame_time = GetFrameTime();
    
    Vector3 cam_dir = Vector3(
                cos(m_camera_pitch) * sin(m_camera_yaw),
                sin(m_camera_pitch),
                cos(m_camera_pitch) * cos(m_camera_yaw));

    cam_dir = Vector3Normalize(cam_dir);
    m_camera_right    = Vector3Normalize(Vector3CrossProduct(cam_dir, AM::UP_VECTOR));
    m_camera_forward  = Vector3Normalize(Vector3CrossProduct(m_camera_right, AM::UP_VECTOR));


    if(IsKeyDown(KEY_LEFT_SHIFT)) {
        m_current_move_speed = m_running_speed;
    }
    else {
        m_current_move_speed = m_walking_speed;
    }

    float move_speed_ft = m_current_move_speed * frame_time;

    if(handle_user_input) {
        if(IsKeyDown(KEY_W)) {
            m_velocity.x -= m_camera_forward.x * move_speed_ft;
            m_velocity.z -= m_camera_forward.z * move_speed_ft;
        }
        else
        if(IsKeyDown(KEY_S)) {
            m_velocity.x += m_camera_forward.x * move_speed_ft;
            m_velocity.z += m_camera_forward.z * move_speed_ft;
        }
        if(IsKeyDown(KEY_A)) {
            m_velocity.x -= m_camera_right.x * move_speed_ft;
            m_velocity.z -= m_camera_right.z * move_speed_ft;
        }
        else
        if(IsKeyDown(KEY_D)) {
            m_velocity.x += m_camera_right.x * move_speed_ft;
            m_velocity.z += m_camera_right.z * move_speed_ft;
        }
    }

    m_position += m_velocity * frame_time;
    this->camera.position = m_position;
    this->camera.target = m_position + cam_dir;
    m_velocity *= pow(m_movement_friction, AM::FRICTION_TCONST * frame_time);
}


 
void AM::Player::update_camera() {
    std::lock_guard<std::mutex> lock(m_mutex);
    Vector2 md = GetMouseDelta();
    m_camera_yaw += (-md.x * m_camera_sensetivity);
    m_camera_pitch += (-md.y * m_camera_sensetivity);
    
    AMutil::clamp<float>(m_camera_pitch, -1.5f, 1.5f);
}
            
void AM::Player::update_position_from_server() {
    if(!m_engine->ready) {
        return;
    }

    if(!Y_pos_update_stack.empty()) {
        m_update_Y_axis_position();

    }
    if(!XZ_pos_update_stack.empty()) {
        m_update_XZ_axis_position();
    }
}

void AM::Player::m_update_Y_axis_position() {
    AM::Timer* ypos_interp_timer = m_engine->get_named_timer("PLAYER_YPOS_INTERP_TIMER");
    float interp_t = ypos_interp_timer->time_ms() / m_engine->net->get_packet_interval_ms(AM::PacketID::PLAYER_POSITION);
    
    std::lock_guard<std::mutex> lock2(m_mutex);
    m_position.y = Lerp(
            // The latest position is added at top of the stack.
            this->Y_pos_update_stack.read_index(1),
            this->Y_pos_update_stack.read_index(0),
            interp_t);
}

void AM::Player::m_update_XZ_axis_position() {
}





