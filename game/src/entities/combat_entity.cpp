#include "entities/combat_entity.h"

#include "state/mining_state.h"

namespace game {

combat_entity::combat_entity(
		mining_state &p_state,
		unsigned int p_initial_health,
		bn::sprite_ptr p_sprite_ptr) :
		entity(p_state),
		_health(p_initial_health),
		_sprite(p_sprite_ptr) {
}

void combat_entity::take_damage(unsigned int p_damage_amount) {
	_state.play_damage_sound();
	if (p_damage_amount >= _health) {
		_health = 0;
	} else {
		_health -= p_damage_amount;
	}
}

bool combat_entity::update() {
	return _health > 0;
}

void combat_entity::set_visible(bool p_visible) {
	_sprite.set_visible(p_visible);
}

} //namespace game
