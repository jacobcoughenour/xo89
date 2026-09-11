#include "scene.h"

#include "bn_bg_palettes.h"
#include "bn_cameras.h"
#include "bn_core.h"
#include "bn_direct_bitmap_item.h"
#include "bn_hbe_ptr.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sp_direct_bitmap_bg_painter.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_tiles.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_vector.h"

#include "items.h"

namespace game {

enum ship_menu {
	TUTORIAL,
	BOUNTIES,
	INVENTORY,
	UPGRADE,
	DEPLOY
};

class ship_scene : public scene {
public:
	explicit ship_scene(shared_state &p_shared);
	~ship_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	bn::camera_ptr _camera;
	bn::sprite_text_generator _small_text;
	bn::optional<bn::sp_direct_bitmap_bg_ptr> _pano_bg;
	bn::vector<bn::sprite_ptr, 180> _text_sprites;
	bn::vector<bn::sprite_ptr, ITEM_TYPE_COUNT * 2> _item_sprites;
	bn::optional<bn::regular_bg_ptr> _screen_bg;
	bn::vector<bn::sprite_ptr, 8> _tutorial_sprites;

	bn::optional<bn::sprite_ptr> _screen_overlay;

	bn::fixed _rotation = 512 / 2;
	ship_menu _selected_ship_menu = ship_menu::TUTORIAL;

	bn::optional<ship_menu> _viewing_menu;

	void _update_tutorial_screen();
	void _update_bounties_screen();
	void _update_inventory_screen();
	void _update_upgrades_screen();

	int _tutorial_page_index = 0;
	int _selected_bounty_index = 0;
	int _selected_upgrade_index = 0;
};
} //namespace game
