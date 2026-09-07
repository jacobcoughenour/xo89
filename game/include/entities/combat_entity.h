#pragma once

#include "entity.h"

#include "bn_fixed_rect.h"
#include "bn_sprite_ptr.h"

namespace game {

class mining_state;

class combat_entity : public entity {
protected:
	combat_entity(mining_state &p_state, unsigned int p_initial_health, bn::sprite_ptr p_sprite_ptr);

	bn::fixed_rect _hitbox;
	unsigned int _health;
	bn::sprite_ptr _sprite;

public:
	virtual void take_damage(unsigned int p_damage_amount);
	bn::fixed_rect get_hitbox() { return _hitbox; }
	bool update() override;
	void set_visible(bool p_visible);
};

} //namespace game