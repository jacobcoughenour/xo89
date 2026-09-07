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
#include "bn_sprite_tiles_ptr.h"

namespace game {

class mining_state;

class turret : public combat_entity {
public:
	explicit turret(
			mining_state &p_state,
			bn::fixed_point p_tile_position,
			unsigned char p_direction);
	bool update() override;

private:
	unsigned int _fire_cooldown;
	unsigned char _direction;
	bn::fixed _aim_angle;
};

} //namespace game
