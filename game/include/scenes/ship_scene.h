#include "scene.h"

#include "bn_bg_palettes.h"
#include "bn_cameras.h"
#include "bn_core.h"
#include "bn_direct_bitmap_item.h"
#include "bn_hbe_ptr.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_sp_direct_bitmap_bg_painter.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "obj_type.h"

namespace game {

enum ship_menu {
	ORDERS,
	INVENTORY,
	UPGRADE,
	DEPLOY,
	// TRAVEL,
};

class ship_scene : public scene {
public:
	explicit ship_scene(shared_state &p_shared);
	~ship_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	bn::camera_ptr _camera;
	bn::sprite_text_generator _small_text;
	bn::sp_direct_bitmap_bg_ptr _pano_bg;
	bn::vector<bn::sprite_ptr, 120> _text_sprites;

	bn::fixed _rotation = 1024 / 2;
	ship_menu _selected_ship_menu = ship_menu::ORDERS;

	bn::optional<ship_menu> _viewing_menu;

	void _update_orders_screen();
	void _update_inventory_screen();
	void _update_upgrades_screen();

	int _selected_bounty_index = 0;
};
} //namespace game
