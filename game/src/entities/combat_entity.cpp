#include "entities/combat_entity.h"

#include "state/mining_state.h"

namespace game {

combat_entity::combat_entity(
		mining_state &p_state,
		unsigned int p_initial_health) :
		entity(p_state),
		_health(p_initial_health) {
}

void combat_entity::take_damage(unsigned int p_damage_amount) {
	if (p_damage_amount >= _health) {
		_health = 0;
	} else {
		_health -= p_damage_amount;
	}
}

bool combat_entity::update() {
	return _health > 0;
}

} //namespace game
