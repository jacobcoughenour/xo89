#pragma once

#include "fonts/common_fixed_8x8_sprite_font.h"
#include "helpers.h"
#include "items.h"
#include "scene.h"
#include "state/mining_state.h"
#include "state/shared_state.h"

#include "entities/floating_item.h"

#include "bn_affine_bg_ptr.h"
#include "bn_camera_ptr.h"
#include "bn_cameras.h"
#include "bn_core.h"
#include "bn_dp_direct_bitmap_bg_painter.h"
#include "bn_dp_direct_bitmap_bg_ptr.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_tiles_items_tiles.h"
#include "bn_seed_random.h"
#include "bn_sound_handle.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_vector.h"

namespace game {

class mining_scene : public scene {
public:
	explicit mining_scene(shared_state &p_shared, mining_state &p_state);
	~mining_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	enum class pause_menu_tab {
		NONE,
		SCANNER,
		INVENTORY,
		BOUNTIES,
		SYSTEM
	};

	inline void append_pause_menu_name(bn::ostringstream &stream, pause_menu_tab p_menu_type) {
		if (p_menu_type == pause_menu_tab::SCANNER) {
			stream.append("SCANNER");
		} else if (p_menu_type == pause_menu_tab::INVENTORY) {
			stream.append("INVENTORY");
		} else if (p_menu_type == pause_menu_tab::BOUNTIES) {
			stream.append("BOUNTIES");
		} else if (p_menu_type == pause_menu_tab::SYSTEM) {
			stream.append("SYSTEM");
		}
	}

	enum class menu_item_options {
		// CONTROLS,
		// MUSIC,
		ABANDON_DRONE,
	};

	menu_item_options _cur_menu_option;
	bool _abandon_selected;

	mining_state &_state;

	pause_menu_tab _pause_tab;
	bn::optional<bn::sprite_ptr> _pause_ship_sprite;
	bn::optional<bn::regular_bg_ptr> _pause_bg;
	bn::vector<bn::sprite_ptr, MAX_BOUNTIES * 2> _pause_item_sprites;

	bn::sprite_text_generator _small_text;

	bn::seed_random _rng;
	unsigned int _frame;

	bn::optional<bn::regular_bg_ptr> _bg_bg;

	alignas(int) bn::regular_bg_map_cell _tilemap_cells[mining_state::TILEMAP_MAX_CELLS];
	bn::regular_bg_map_item _tilemap_item;
	bn::optional<bn::regular_bg_item> _tilemap_bg_item;
	bn::optional<bn::regular_bg_ptr> _tilemap_bg;
	bn::point _tilemap_loaded_point;

	bn::optional<bn::sprite_ptr> _ship_sprite;

	struct particle_lifetime {
		bn::sprite_ptr sprite;
		unsigned char time;
		bn::fixed_point velocity;
	};
	bn::vector<particle_lifetime, 5> _ship_thrust_particles;
	unsigned char _next_thrust_particle = 0;
	unsigned char _thrust_particle_time = 0;

	bn::optional<bn::affine_bg_ptr> _ship_laser;

	bn::sprite_ptr _breaking_sprite;
	bn::sprite_ptr _crosshair_sprite;
	int _crosshair_frame;

	bn::vector<bn::sprite_ptr, 80> _text_sprites;

	bn::optional<bn::dp_direct_bitmap_bg_ptr> _scan_map_bg;

	bn::vector<bn::sprite_ptr, mining_state::MAX_VISIBLE_FLOATING_ITEMS> _floating_item_sprites;
	bn::vector<bn::sprite_ptr, mining_state::MAX_VISIBLE_PROJECTILES> _proj_sprites;
	int _obj_flicker_frame;

	void _pause(bool p_show_radar);
	void _unpause();

	void _update_overlay_text();
	void _update_space();
	void _update_pause_menu();

	void _update_tilemap();
	void _set_tilemap_tile(int seed, int p_x, int p_y, int p_edge_mask, tile_material p_material, unsigned char p_light_level);
};

} //namespace game