#pragma once

#include "bn_fixed_rect.h"
#include "entity.h"

namespace game {

class mining_state;

class combat_entity : public entity {
protected:
	combat_entity(mining_state &p_state, unsigned int p_initial_health);

	bn::fixed_rect _hitbox;
	unsigned int _health;

public:
	void take_damage(unsigned int p_damage_amount);
	bn::fixed_rect get_hitbox() { return _hitbox; }
	bool update() override;
};

} //namespace game