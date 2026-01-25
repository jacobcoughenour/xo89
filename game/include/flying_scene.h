#ifndef FLYING_SCENE_H
#define FLYING_SCENE_H

#include "game_state.h"
#include "helpers.h"
#include "scene.h"

#include "bn_affine_bg_ptr.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
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

	// do we want to combine these into a "transform"
	bn::sprite_ptr _ship_sprite;
	bn::fixed_point _ship_position;
	bn::fixed_point _ship_velocity;
	bn::fixed _ship_rotation;

	bn::vector<bn::sprite_ptr, 32> _floating_objs;

	bn::vector<bn::sprite_ptr, 16> _text_sprites;
};
} //namespace game
#endif