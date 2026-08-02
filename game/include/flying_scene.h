#pragma once

#include "chunked_space.h"
#include "game_state.h"
#include "helpers.h"
#include "scene.h"

#include "bn_affine_bg_ptr.h"
#include "bn_cameras.h"
#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"

namespace game {

class flying_scene : public scene {
public:
	explicit flying_scene(game_state &state);
	~flying_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

	void update_text();

private:
	game_state &_state;

	bn::camera_ptr _camera;
	bn::seed_random _rng;
	int _frame;

	bn::point _laser_target_cell;
	int _mining_timer;

	bn::fixed_rect _ship_hitbox;

	bn::regular_bg_ptr _bg_bg;

	chunked_space _space;

	// do we want to combine these into a "transform"
	bn::sprite_ptr _ship_sprite;
	bn::fixed_point _ship_velocity;
	bn::fixed _ship_rotation;

	bn::affine_bg_ptr _ship_laser;

	bn::sprite_ptr _test_sprite;

	// bn::vector<bn::sprite_ptr, 32> _floating_spites;
	// bn::vector<bn::sprite_ptr, 8> _projectiles;

	bn::vector<bn::sprite_ptr, 16> _text_sprites;
};
} //namespace game