#ifndef AMBIENT3D_ITEM_HPP
#define AMBIENT3D_ITEM_HPP

#include <memory>

#include "item_base.hpp"
#include "renderable.hpp"
#include "raylib.h"


namespace AM {

    // TODO: Rename to DroppedItem

    class Item : public AM::ItemBase {
        public:

            std::shared_ptr<AM::Renderable> renderable;

            // How long the item has been far away from the player.
            // ItemManager uses this to unload unused items.
            float inactive_time;

        private:

    };

};



#endif
