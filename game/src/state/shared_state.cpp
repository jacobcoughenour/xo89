#include "state/shared_state.h"

namespace game {

shared_state::shared_state() {
	generate_bounties();

	_ship_inventory[static_cast<int>(obj_type::GOLD)] = 1000;
}

void shared_state::update() {
	_frames++;
}

void shared_state::generate_bounties() {
	_bounties.clear();

	_rng.set_seed(_frames);

	auto count = 3 + _rng.get_int(2);
	for (int i = 0; i < count; i++) {
		int type = _rng.get_int(static_cast<int>(obj_type::OBJ_TYPE_MAX));

		unsigned int per_price = 2 + _rng.get_int(8);
		unsigned int amount = 30 + _rng.get_int(50);

		bounty b{
			static_cast<obj_type>(type),
			// todo weights
			amount,
			amount * per_price,
		};
		_bounties.push_back(b);
	}
}

int shared_state::get_inventory_count(obj_type p_obj_type) {
	return _ship_inventory[static_cast<int>(p_obj_type)];
}

void shared_state::deposit_to_inventory(obj_type p_obj_type, unsigned int p_amount) {
	_ship_inventory[static_cast<int>(p_obj_type)] += p_amount;
}

void shared_state::_withdraw_from_inventory(obj_type p_obj_type, unsigned int p_amount) {
	BN_ASSERT(_ship_inventory[static_cast<int>(p_obj_type)] >= p_amount);
	_ship_inventory[static_cast<int>(p_obj_type)] -= p_amount;
}

unsigned int shared_state::get_frame_count() {
	return _frames;
}

int shared_state::get_balance() {
	return _balance;
}
const bounty_list &shared_state::get_bounties() {
	return _bounties;
}

bool shared_state::collect_bounty(int p_bounty_index) {
	if (p_bounty_index >= _bounties.size()) {
		return false;
	}

	auto &selected = _bounties[p_bounty_index];

	if (selected.collected) {
		return false;
	}

	auto count = get_inventory_count(selected.resource);
	if (count < selected.amount) {
		// insufficient amount in inventory
		return false;
	}

	selected.collected = true;

	_withdraw_from_inventory(selected.resource, selected.amount);
	_balance += selected.price;

	return true;
}
} //namespace game
