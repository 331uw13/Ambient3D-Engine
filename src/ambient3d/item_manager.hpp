#ifndef AMBIENT3D_ITEM_MANAGER_HPP
#define AMBIENT3D_ITEM_MANAGER_HPP


// Item manager handles loading and unloading received dropped items.
// Their 3D models, inventory textures and item type specific information.
//
// The renderables(models) are stored in unordered map.
// So the renderable for the item can be accessed with item id.
// Renderable for the item may be loaded when server will notice player is nearby.
// Also it can be unloaded when its shared_ptr use_count reaches 1.


#include <map>
#include <array>
#include <nlohmann/json.hpp>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "raylib.h"
#include "item.hpp"
#include "server_config.hpp"

using json = nlohmann::json;


namespace AM {

    class ItemManager {
        public:

            ItemManager();
            ~ItemManager();
            void free();

            void cleanup_unused_items(const Vector3& player_pos);     // < thread safe >
            void add_itembase_to_queue(const AM::ItemBase& itembase); // < thread safe >
            void add_itemuuid_removed(int item_uuid);                 // < thread safe >

            // IMPORTANT NOTE: must be called from main thread.
            void update_items_queue();

            void set_item_default_shader(const Shader& shader) { m_item_default_shader = shader; }
            void set_server_config(const AM::ServerCFG& server_cfg) { m_server_cfg = server_cfg; }
            void set_item_list(const json& item_list) { m_item_list_json = item_list; }

            // TODO: Make this thread safe...
            const std::unordered_map<int/*item_uuid*/, AM::Item>* 
                get_dropped_items() { return &m_dropped_items; }

        private:

            void     m_load_item_data(AM::ItemBase* itembase);
            void     m_update_item_data(AM::ItemBase* itembase);

            Shader                   m_item_default_shader;
            json                     m_item_list_json;
            AM::ServerCFG            m_server_cfg;

            std::mutex               m_itembase_queue_mutex;
            std::deque<AM::ItemBase> m_itembase_queue;
           
            std::vector<int/*item_uuid*/> m_removed_itemuuid_queue;

            // Item renderables(models) are loaded
            // if client receives items (see ITEM_UPDATE packet)
            // and is not loaded yet.
            
            std::array<std::shared_ptr<AM::Renderable>, AM::NUM_ITEMS> m_item_renderables;
            std::unordered_map<int/* item uuid */, AM::Item> m_dropped_items;

    };


};


#endif
