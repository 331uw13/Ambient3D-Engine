#ifndef AMBIENT3D_INVENTORY_HPP
#define AMBIENT3D_INVENTORY_HPP

#include <cstdint>

#include "item_base.hpp"


namespace AM {

    struct InventorySize {
        uint8_t num_slots_x;
        uint8_t num_slots_y;

        uint16_t num_total_slots { 0 };
    };

    class Inventory {
        public:

            void create(const InventorySize& size);
            void free_memory();

            // Adds item to first empty slot.
            // Returns 'false' if inventory is full or was not created.
            // Otherwise returns 'true'
            bool add_itembase(const AM::ItemBase& itembase);
            
            // May return NULL if item was not found.
            AM::ItemBase* get_item_from_xy(uint8_t slot_x, uint8_t slot_y);
            AM::ItemBase* get_item_from_index(uint16_t index);

            // Returns 'false' if item was not found.
            // Otherwise returns 'true'
            bool remove_item_from_xy(uint8_t slot_x, uint8_t slot_y);
            bool remove_item_from_index(uint16_t index);

            InventorySize get_size() { return m_size; }

        private:

            InventorySize m_size;
            AM::ItemBase* m_items { NULL };

    };


};

#endif
