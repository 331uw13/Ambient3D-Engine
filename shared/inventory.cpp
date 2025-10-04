#include <cstdio>

#include "inventory.hpp"

            

void AM::Inventory::create(const InventorySize& size) {
    if(m_items) {
        fprintf(stderr, "ERROR! %s() at %s: Inventory cant be re-created\n",
                __func__, __FILE__);
        return;
    }

    m_size = size;
    if(m_size.num_slots_x == 0) {
        m_size.num_slots_x = 1;
    }
    if(m_size.num_slots_y == 0) {
        m_size.num_slots_y = 1;
    }

    m_size.num_total_slots = m_size.num_slots_x * m_size.num_slots_y;
    m_items = new AM::ItemBase[m_size.num_total_slots];
}
            
void AM::Inventory::free_memory() {
    if(!m_items) {
        return;
    }
    delete[] m_items;
    m_items = NULL;
}

bool AM::Inventory::add_itembase(const AM::ItemBase& itembase) {
    if(!m_items) {
        return false;
    }

    for(uint16_t i = 0; i < m_size.num_total_slots; i++) {
        AM::ItemBase& inv_itembase = m_items[i];
        if(inv_itembase.state == AM::ItemState::DROPPED) {
            inv_itembase = itembase;
            inv_itembase.state = AM::ItemState::IN_INVENTORY;
            return true;
        }
    }

    return false;
}

AM::ItemBase* AM::Inventory::get_item_from_xy(uint8_t slot_x, uint8_t slot_y) {
    return this->get_item_from_index(slot_y * m_size.num_slots_x + slot_x);
}

AM::ItemBase* AM::Inventory::get_item_from_index(uint16_t index) {
    if(!m_items) { 
        return NULL;
    }
    if(index >= m_size.num_total_slots) {
        return NULL;
    }

    AM::ItemBase* itembase = &m_items[index];
    if(itembase->state == AM::ItemState::DROPPED) {
        return NULL;
    }

    return itembase;
}

bool AM::Inventory::remove_item_from_xy(uint8_t slot_x, uint8_t slot_y) {
    return this->remove_item_from_index(slot_y * m_size.num_slots_x + slot_x);
}

bool AM::Inventory::remove_item_from_index(uint16_t index) {
    if(!m_items) {
        return false;
    }
    if(index >= m_size.num_total_slots) {
        return false;
    }

    m_items[index].state = AM::ItemState::DROPPED;
    return true;
}


