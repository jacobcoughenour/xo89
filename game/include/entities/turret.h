#pragma once

#include "combat_entity.h"
#include "entity.h"
#include "helpers.h"
#include "items.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_fixed_rect.h"
#include "bn_point.h"
#include "bn_sprite_ptr.h"

namespace game {

class mining_state;

class turret : public combat_entity {
public:
	explicit turret(
			mining_state &p_state,
			bn::fixed_point p_tile_position);
	bool update() override;

private:
	bn::sprite_ptr _sprite;
	unsigned int _fire_cooldown;
};

} //namespace game
