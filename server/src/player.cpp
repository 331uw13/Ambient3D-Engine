#include "player.hpp"
#include "server.hpp"




AM::Player::Player(std::shared_ptr<TCP_session> _tcp_session)
: tcp_session(std::move(_tcp_session)) 
{
}



int AM::Player::id() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_id;
}

void AM::Player::set_id(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_id = id;
}

float AM::Player::cam_yaw() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cam_yaw;
}

void AM::Player::set_cam_yaw(float yaw) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cam_yaw = yaw;
}

float AM::Player::cam_pitch() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cam_pitch;
}

void AM::Player::set_cam_pitch(float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cam_pitch = pitch;
}

float AM::Player::terrain_surface_y() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_terrain_surface_y;
}

/*
void AM::Player::set_terrain_surface_y(float y) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_terrain_surface_y = y;
}
*/

AM::Vec3 AM::Player::position() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_position;
}

void AM::Player::set_position(const AM::Vec3& p) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_position = p;
}



AM::Vec3 AM::Player::velocity() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_velocity;
}

void AM::Player::set_velocity(const AM::Vec3& v) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_velocity = v;
}

AM::ChunkPos AM::Player::chunk_pos() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_chunk_pos;
}

int AM::Player::animation_id() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_animation_id;
}

void AM::Player::set_animation_id(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_animation_id = id;
}



AM::Vec3 AM::Player::next_position() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_next_position;
}

void AM::Player::set_next_position_Y(float y) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_next_position.y = y;
    m_next_position_flags |= FLG_PLAYER_UPDATE_Y_AXIS;
}

void AM::Player::set_next_position_XYZ(const AM::Vec3& p) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_next_position = p;
    m_next_position_flags |= FLG_PLAYER_UPDATE_Y_AXIS;
    m_next_position_flags |= FLG_PLAYER_UPDATE_XZ_AXIS;
}

            
int AM::Player::next_position_flags() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_next_position_flags;
}

void AM::Player::clear_next_position_flags() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_next_position_flags = 0;
}


       
void AM::Player::update() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_terrain_surface_y = m_server->terrain.get_surface_level(m_position);
    m_chunk_pos = m_server->terrain.get_chunk_pos(m_position.x, m_position.z);
        
    this->on_ground = ((m_position.y - (m_server->config.player_cam_height)) < m_terrain_surface_y);
}


