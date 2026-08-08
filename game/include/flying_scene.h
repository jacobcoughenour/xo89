#pragma once

#include "chunked_space.h"
#include "game_state.h"
#include "helpers.h"
#include "obj_type.h"
#include "scene.h"

#include "bn_affine_bg_ptr.h"
#include "bn_cameras.h"
#include "bn_core.h"
#include "bn_dp_direct_bitmap_bg_painter.h"
#include "bn_dp_direct_bitmap_bg_ptr.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"

namespace game {

enum class PAUSE_MENU_TAB {
	NONE,
	RESOURCES,
	MAP,
	SHIP
};

class flying_scene : public scene {
public:
	explicit flying_scene(game_state &state);
	~flying_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	bool _is_paused;

	void _update_overlay_text();
	void _update_space();
	void _update_pause_menu();
	void _rebuild_bgs();
	void _destroy_bgs();

	game_state &_state;

	bn::camera_ptr _camera;
	bn::seed_random _rng;
	int _frame;

	bn::point _laser_target_cell;
	int _mining_timer;

	bn::optional<bn::regular_bg_ptr> _bg_bg;

	chunked_space _space;

	// do we want to combine these into a "transform"?
	bn::fixed_rect _ship_hitbox;
	bn::fixed _ship_rotation = 180;
	bn::fixed_point _ship_velocity;
	bn::sprite_ptr _ship_sprite;
	bn::vector<bn::sprite_ptr, 3> _ship_thrust_particles;

	bn::optional<bn::affine_bg_ptr> _ship_laser;

	bn::sprite_ptr _breaking_sprite;
	bn::sprite_ptr _crosshair_sprite;
	int _crosshair_frame;

	bn::vector<bn::sprite_ptr, 16> _text_sprites;

	bn::optional<bn::dp_direct_bitmap_bg_ptr> _scan_map_bg;
};
} //namespace game