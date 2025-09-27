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

        // Client must tell the server it has been fully connected
        // after it stored all needed information.
        PLAYER_FULLY_CONNECTED, // (tcp only)

        // --------- End Connection packets -----------


        // World generation is server side.
        // This packet can send chunk data to client.
        // TODO: It is probably good idea to compress these packets in the future.
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
        // 8            :  Player pos X     (float)
        // 12           :  Player pos Y     (float)
        // 16           :  Player pos Z     (float)
        // 20           :  Camera Yaw       (float)
        // 24           :  Camera Pitch     (float)
        // 28           :  Animation ID     (int)
        PLAYER_MOVEMENT_AND_CAMERA, // (udp only)

        // Server is going to send player their position.
        // This is because sometimes the player may collide with something
        // and the position must be changed from the server.
        //
        // Byte offset  |  Value name
        // ---------------------------------
        // 0            :  Packet ID        (int)
        // 4            :  On ground        (bool)
        // 8            :  Chunk X          (int)
        // 12           :  Chunk Z          (int)
        // 16           :  Update XZ axis   (bool)
        //
        //              if "Update XZ axis" is true the full position is sent.
        //
        // 20           :  X Position       (float)
        // 24           :  Y Position       (float)
        // 28           :  Z Position       (float)
        //
        //              The packet may also contain only the Y position.
        //
        // 20           :  Y Position       (float)
        //
        PLAYER_POSITION, // (udp only)

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
      


        NUM_PACKETS
    };

    namespace PacketSize {
        static constexpr size_t PLAYER_MOVEMENT_AND_CAMERA = 28;
        static constexpr size_t PLAYER_ID = 4;
        static constexpr size_t PLAYER_CONNECTED = 4;
        static constexpr size_t PLAYER_UNLOADED_CHUNK = 8;
        static constexpr size_t PLAYER_POSITION_MIN = 20;
        static constexpr size_t PLAYER_POSITION_MAX = 28;
        static constexpr size_t PLAYER_UNLOADED_CHUNKS_MIN = 8;
    };
};




#endif
