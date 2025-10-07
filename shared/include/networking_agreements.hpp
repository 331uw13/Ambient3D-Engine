#ifndef AMBIENT3D_NETWORKING_AGREEMENTS_HPP
#define AMBIENT3D_NETWORKING_AGREEMENTS_HPP

#include <cstdint>


namespace AM {
    
    static constexpr uint8_t PACKET_DATA_SEPARATOR = 0x1F;
    static constexpr uint8_t PACKET_DATA_STOP = 0x3;
    static constexpr size_t MAX_PACKET_SIZE = 1024 * 34;

    // For AM::PacketID::PLAYER_POSITION 'update_axis'
    static constexpr int FLG_PLAYER_UPDATE_Y_AXIS = (1 << 0);
    static constexpr int FLG_PLAYER_UPDATE_XZ_AXIS = (1 << 1);

};

#endif
