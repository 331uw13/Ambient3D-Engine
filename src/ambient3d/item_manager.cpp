
#include <iostream>
#include "item_manager.hpp"
#include "raymath.h"

            
AM::ItemManager::ItemManager() {
    for(size_t i = 0; i < m_item_renderables.size(); i++) {
        m_item_renderables[i] = std::make_shared<AM::Renderable>(AM::Renderable{});
    }
}
            
AM::ItemManager::~ItemManager() {
    for(size_t i = 0; i < m_item_renderables.size(); i++) {
        AM::Renderable* renderable = m_item_renderables[i].get();
        if(renderable->is_loaded()) {
            renderable->unload();
        }
    }
}


void AM::ItemManager::cleanup_unused_items(const Vector3& player_pos) { 
    m_itembase_queue_mutex.unlock();
    
    for(auto it = m_dropped_items.begin(); it != m_dropped_items.end(); ) {
        AM::Item* item = &it->second;
        std::shared_ptr<AM::Renderable>& renderable_ptr = m_item_renderables[item->id];
        const float dist_to_player = Vector3Distance(player_pos, Vector3(item->pos_x, item->pos_y, item->pos_z));
        

        if(((dist_to_player > m_server_cfg.item_near_distance)
        || (renderable_ptr.use_count() == 1))
        && renderable_ptr->is_loaded()) {
            renderable_ptr->unload();
            it = m_dropped_items.erase(it);
            printf("[ITEM_MANAGER]: Unloaded item \"%s\"\n", item->entry_name);
        }
        else {
            ++it;
        }
    }
}
 
void AM::ItemManager::m_load_item_data(AM::ItemBase* itembase) {
    itembase->load_info(m_item_list_json, itembase->id, itembase->entry_name);
    AM::Item item = (AM::Item)*itembase;
    

    AM::Renderable* renderable = m_item_renderables[item.id].get();
    if(!renderable->is_loaded()) {
        if(!renderable->load(item.model_path, { m_item_default_shader })) {
            fprintf(stderr, "[ITEM_MANAGER]: Failed to load item model \"%s\" for \"%s\"\n",
                    item.model_path, item.entry_name);
            return;
        }

        json model_settings = m_item_list_json[item.entry_name]["modelmesh_settings"];
        
        size_t mesh_index = 0;
        for(const json& mesh_settings : model_settings) {
            std::cout << mesh_settings << std::endl;
            
            const int hex_color = strtol(mesh_settings["color"].template get<std::string>().c_str(), NULL, 16);
            
            renderable->mesh_attribute(mesh_index, 
                    AM::MeshAttrib {
                        .tint = Color((hex_color >> 16) & 0xFF, // Red
                                      (hex_color >> 8)  & 0xFF, // Green
                                      (hex_color)       & 0xFF, // Blue
                                      255),                     // Alpha
                        .shine = mesh_settings["shine"].template get<float>(),
                        .specular = mesh_settings["specular"].template get<float>()
                    });

            mesh_index++;
            if(mesh_index > renderable->num_meshes()) {
                fprintf(stderr, "ERROR! %s: Renderable mesh count is smaller than"
                        " number of elements in \"modelmesh_settings\" in item_list.json (%s)\n",
                        __func__, item.entry_name);
                break;
            }
        }
    }

    //printf("USE_COUNT BEFORE MOVE = %li\n", m_item_renderables[item.id].use_count());
    item.renderable = m_item_renderables[item.id];
    //printf("USE_COUNT AFTER MOVE = %li\n", m_item_renderables[item.id].use_count());

    m_dropped_items.insert(std::make_pair(item.uuid, item));
    printf("[ITEM_MANAGER]: Loaded item \"%s\" %i\n", itembase->entry_name, itembase->uuid);
                    
}

void AM::ItemManager::m_update_item_data(AM::ItemBase* itembase) {

}


void AM::ItemManager::update_items_queue() {
    std::lock_guard<std::mutex> lock(m_itembase_queue_mutex);
    for(size_t i = 0; i < m_itembase_queue.size(); i++) {
        AM::ItemBase* itembase = &m_itembase_queue[i];
        if(itembase->id >= AM::NUM_ITEMS) {
            continue;
        }

        auto item_search = m_dropped_items.find(itembase->uuid);
        if(item_search != m_dropped_items.end()) {
            m_update_item_data(itembase);
        }
        else {
            m_load_item_data(itembase);
        }
    }

    m_itembase_queue.clear();


    // Remove unloaded items.
    for(size_t i = 0; i < m_removed_itemuuid_queue.size(); i++) {
        auto item_search = m_dropped_items.find(m_removed_itemuuid_queue[i]);
        if(item_search == m_dropped_items.end()) {
            continue;
        }

        m_dropped_items.erase(item_search);
    }

    m_removed_itemuuid_queue.clear();
}

void AM::ItemManager::add_itembase_to_queue(const AM::ItemBase& itembase) {
    std::lock_guard<std::mutex> lock(m_itembase_queue_mutex);
    m_itembase_queue.push_back(itembase);
}

            
void AM::ItemManager::add_itemuuid_removed(int item_uuid) {
    std::lock_guard<std::mutex> lock(m_itembase_queue_mutex);
    m_removed_itemuuid_queue.push_back(item_uuid);
}

