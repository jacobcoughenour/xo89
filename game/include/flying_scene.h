#ifndef FLYING_SCENE_H
#define FLYING_SCENE_H

#include "scene.h"
#include "game_state.h"

namespace game {

class flying_scene : public scene {
public:
	explicit flying_scene(game_state &state);
	~flying_scene();
	[[nodiscard]] bn::optional<scene_type> update() final;

private:
	game_state &_state;

	// do we want to combine these into a "transform"
	bn::sprite_ptr _ship_sprite;
	bn::fixed_point _ship_position;
	bn::fixed _ship_rotation;

	bn::vector<bn::sprite_ptr, 32> _floating_objs;

};
} //namespace game
#endif