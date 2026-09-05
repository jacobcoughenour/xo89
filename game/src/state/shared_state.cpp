#include "state/shared_state.h"

namespace game {

shared_state::shared_state() {
	bn::sram::read(_saved_data);
}

void shared_state::update() {
	_frames++;
}

bool shared_state::has_save() {
	return _saved_data.format == 1;
}

void shared_state::new_game() {
	_saved_data.format = 1;
	_saved_data.balance = 0;
	for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
		_saved_data.ship_inventory[i] = 0;
	}
	generate_bounties();
}

void shared_state::load() {
	if (_saved_data.format != 1) {
		BN_LOG("format = ", _saved_data.format);
		BN_LOG("Failed to read save data. Creating new game.");
		new_game();
		save();
	}
	generate_bounties();
}

void shared_state::save() {
	bn::sram::write(_saved_data);
}

void shared_state::generate_bounties() {
	_bounties.clear();

	_rng.set_seed(_frames);

	auto count = 3 + _rng.get_int(2);
	for (int i = 0; i < count; i++) {
		int type = _rng.get_int(static_cast<int>(item_type::ITEM_TYPE_MAX));

		unsigned int per_price = 2 + _rng.get_int(8);
		unsigned int amount = 30 + _rng.get_int(50);

		bounty b{
			.resource = static_cast<item_type>(type),
			// todo weights
			.amount = amount,
			.price = amount * per_price,
			.collected = false
		};
		_bounties.push_back(b);
	}
}

unsigned int shared_state::get_inventory_count(item_type p_item_type) {
	return _saved_data.ship_inventory[static_cast<int>(p_item_type)];
}

void shared_state::deposit_to_inventory(item_type p_item_type, unsigned int p_amount) {
	_saved_data.ship_inventory[static_cast<int>(p_item_type)] += p_amount;
}

void shared_state::_withdraw_from_inventory(item_type p_item_type, unsigned int p_amount) {
	BN_ASSERT(_saved_data.ship_inventory[static_cast<int>(p_item_type)] >= p_amount);
	_saved_data.ship_inventory[static_cast<int>(p_item_type)] -= p_amount;
}

unsigned int shared_state::get_frame_count() {
	return _frames;
}

int shared_state::get_balance() {
	return _saved_data.balance;
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
	_saved_data.balance += selected.price;

	save();

	return true;
}

} //namespace game
