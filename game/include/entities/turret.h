#pragma once

#include "entity.h"
#include "helpers.h"
#include "items.h"
#include "targetable.h"

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_fixed_rect.h"
#include "bn_point.h"
#include "bn_sprite_ptr.h"

namespace game {

class mining_state;

class turret : public entity, public targetable {
public:
	explicit turret(
			mining_state &p_state,
			bn::fixed_point p_tile_position);
	bool update() override;

private:
	bn::sprite_ptr _sprite;
	bn::fixed_rect _hitbox;
	unsigned int _health;
	unsigned int _fire_cooldown;

public:
	void take_damage(unsigned int p_damage_amount);
	bn::fixed_rect get_hitbox() { return _hitbox; }
};

} //namespace game
