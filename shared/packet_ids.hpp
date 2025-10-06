#ifndef AMBIENT3D_PACKET_IDS_HPP
#define AMBIENT3D_PACKET_IDS_HPP



// Client and server will send a packet
// with this kind of format:
//
// <TCP/UDP packet id> (4 bytes reserved.)
// Packet id is followed by collection of data,
//
// The data may sometimes be separated by 'PACKET_DATA_SEPARATOR'
// If data size may not be fixed. 
// But this depends alot on what kind of data is being sent/received
//

namespace AM {


    enum PacketID : int {
        NONE = 0,

        CHAT_MESSAGE,    // (tcp only)
        SERVER_MESSAGE,  // (tcp only)


        // ----------- Connection packets -------------

        /* Connection to server will work like this:

            Sender/Receiver  |  PacketID  |  Protocol
            -------------------------------------------------
            Server -> Client  (PLAYER_ID)                (TCP)
            Server <- Client  (PLAYER_ID)                (UDP)
            Server -> Client  (PLAYER_ID_HAS_BEEN_SAVED) (TCP)
            Server <- Client  (PLAYER_CONNECTED)         (TCP)
            Server -> Client  (SAVE_ITEM_LIST)           (TCP)
            Server <- Client  (GET_SERVER_CONFIG)        (TCP)
            Server -> Client  (SERVER_CONFIG)            (TCP)
            Server <- Client  (CLIENT_CONFIG)            (TCP)
            Server -> Client  (TIMEOFDAY_SYNC)           (TCP)
            Server <- Client  (PLAYER_FULLY_CONNECTED)   (TCP)
        */

        // Server will send player their id when they are connected (via TCP)
        // Then the client will reply with their ID via UDP protocol,
        // that process saves the client's udp endpoint.
        // If the server doesnt receive response from client via UDP
        // it will add the client to a queue for trying again.
        PLAYER_ID,

        // ^-- After that if successful the server will respond with
        // this packet via TCP
        PLAYER_ID_HAS_BEEN_SAVED,

        // ^-- After client got PLAYER_ID_HAS_BEEN_SAVED.
        // it will send this packet and then wait for SAVE_ITEM_LIST packet to arrive.
        PLAYER_CONNECTED, // (tcp only)
        
        // Server is going to send full list of items to client when they connect.
        // The client should save the list for future use.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  JSON data        (char array)
        //
        SAVE_ITEM_LIST, // (tcp only)
       
        // Client can ask the server for its configuration
        // for many purposes.
        GET_SERVER_CONFIG, // (tcp only)

        // ^-- Server should repond to that packet with this one:
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Config json      (char array)
        SERVER_CONFIG,  // (tcp only)

        // Client will sent its config to the server.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Config json      (char array)
        CLIENT_CONFIG,  // (tcp only)
       
        // Client must tell the server it has been fully connected
        // after it stored all needed information.
        PLAYER_FULLY_CONNECTED, // (tcp only)

        // --------- End Connection packets -----------


        // World generation is server side.
        // This packet can send chunk data to client.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Chunk X          (int)
        // 8            :  Chunk Z          (int)
        // 12           :  Chunk data       (float array)
        // 
        // NOTES:
        // The packet may contain more than one chunk.
        // The chunk data size is equal to (server_config.chunk_size * server_config.chunk_size)
        CHUNK_DATA, // (udp only)

        // Player must send this packet to server when they unload chunks
        // because the server stores what chunks it has sent to the player.
        // So if the player decides to go back they will be loaded again.
        // This packet can also be used to trigger resending of chunks.
        // 
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Chunk X          (int)
        // 8            :  Chunk Z          (int)
        //
        // NOTES:
        // The packet may contain more than one chunk position.
        // Next chunk position starts at byte offset 8
        PLAYER_UNLOADED_CHUNKS, // (tcp only)

        // This packet contains the player's position in the world.
        // It also has camera pitch, yaw and animation information.
        // The client side position is always a "request" to the server.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Player ID        (int)
        // 8            :  Animation ID     (int)
        // 12           :  Player pos X     (float)
        // 16           :  Player pos Y     (float)
        // 20           :  Player pos Z     (float)
        // 24           :  Camera Yaw       (float)
        // 28           :  Camera Pitch     (float)
        PLAYER_MOVEMENT_AND_CAMERA, // (udp only)

        // This packet can be used by the server to set player's positions.
        // Can be used with collision checks so player doesnt clip into something.
        // For example: Y position is usually used for terrain surface.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  On ground        (bool)
        // 8            :  Chunk X          (int)
        // 12           :  Chunk Z          (int)
        // 16           :  Update axis      (int) (0 or flags: AM::UPDATE_PLAYER_Y_AXIS, AM::UPDATE_PLAYER_XZ_AXIS)
        //
        //
        //  If (update_axis & AM::UPDATE_PLAYER_Y_AXIS)
        //  then rest of packet contains:
        //
        // 20           :  Y Position       (float)  
        //
        //  If (update_axis & AM::UPDATE_PLAYER_XZ_AXIS)
        //  then rest of packet contains:
        //
        // 20           :  X Position       (float)
        // 24           :  Z Position       (float)
        //
        //  If ((update_axis & AM::UPDATE_PLAYER_XZ_AXIS) && (update_axis & AM::UPDATE_PLAYER_Y_AXIS))
        //  then rest of packet contains:
        //
        // 20           :  X Position       (float)
        // 24           :  Y Position       (float)
        // 28           :  Z Position       (float)
        //
        // 
        //  If update_axis == 0:
        //  then rest of the packet doesnt contain anything else.
        //
        PLAYER_POSITION, // (udp only)

        // Players can send jump packet to server.
        // 
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Player ID        (int)
        PLAYER_JUMP, // (udp only)

        // Server will send item update packets to let nearby 
        // clients know where the items are.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Item UUID        (int)
        // 8            :  Item ID          (int)
        // 12           :  Item pos X       (float)
        // 16           :  Item pos Y       (float)
        // 20           :  Item pos Z       (float)
        // 24           :  Item entry name  (char array)
        //
        // NOTES:
        // The packet may contain multiple items.
        // To separate items 'AM::PACKET_DATA_SEPARATOR' 
        // is used after (byte offset 24 + item entry name size)
        // AM::PACKET_DATA_SEPARATOR is defined in "networking_agreement.hpp"
        ITEM_UPDATE, // (udp only)
     
        // Clients handle their own time but when the client connects
        // it has to know what time of day it is.
        // And every 10 seconds server sends this packet to clients which are already connected.
        // (This is in game time and not related to real life time at all)
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Time of day      (float)
        TIMEOFDAY_SYNC, // (tcp only)

        // Server sends players the weather data.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Fog density      (float)
        // 8            :  Fog Color R      (float)
        // 12           :  Fog Color G      (float)
        // 16           :  Fog Color B      (float)
        WEATHER_DATA, // (udp only)


        // Clients can send this packet to pickup items.
        // The server will check if the pickup is allowed.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  Item UUID        (int)
        PICKUP_ITEM,


        NUM_PACKETS
    };

    namespace PacketSize {
        static constexpr size_t PLAYER_MOVEMENT_AND_CAMERA = 28;
        static constexpr size_t PLAYER_ID = 4;
        static constexpr size_t PLAYER_CONNECTED = 4;
        static constexpr size_t PLAYER_UNLOADED_CHUNK = 8;
        static constexpr size_t PLAYER_POSITION_MIN = 16;
        static constexpr size_t PLAYER_POSITION_MAX = 28;
        static constexpr size_t PLAYER_UNLOADED_CHUNKS_MIN = 8;
        static constexpr size_t PLAYER_JUMP = 4;
        static constexpr size_t TIMEOFDAY_SYNC = 4;
        static constexpr size_t WEATHER_DATA = 16;
    };
};




#endif
