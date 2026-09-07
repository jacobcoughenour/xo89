#pragma once

#include "combat_entity.h"
#include "helpers.h"
#include "items.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_fixed_rect.h"
#include "bn_point.h"
#include "bn_sprite_ptr.h"

namespace game {

class mining_state;

class creep : public combat_entity {
public:
	explicit creep(
			mining_state &p_state,
			bn::fixed_point p_spawn_position);
	bool update() override;

private:
	unsigned int _exploding_timer;

	void _explode();
};

} //namespace game
