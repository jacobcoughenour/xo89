#include "state/shared_state.h"

#include "bn_sound_items.h"

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
	for (int i = 0; i < UPGRADE_COUNT; i++) {
		_saved_data.upgrade_levels[i] = 0;
	}
	_saved_data.upgrade_slots = 0;
	_saved_data.has_rocket_launcher = false;
	generate_bounties();
	_loaded = true;
}

void shared_state::load() {
	if (_saved_data.format != 1) {
		BN_LOG("format = ", _saved_data.format);
		BN_LOG("Failed to read save data. Creating new game.");
		new_game();
		save();
	}
	generate_bounties();
	_loaded = true;
}

void shared_state::save() {
	bn::sram::write(_saved_data);
}

void shared_state::ensure_loaded() {
	if (!_loaded) {
		new_game();
	}
}

void shared_state::generate_bounties() {
	_bounties.clear();
	_rng.set_seed(_frames);
	while (!_bounties.full()) {
		_generate_bounty();
	}
}

void shared_state::_generate_bounty() {
	auto type = _rng.get_int(ITEM_TYPE_COUNT);

	for (auto it = _bounties.begin(); it != _bounties.end(); ++it) {
		// try to avoid duplicates
		if (it->resource == static_cast<item_type>(type)) {
			type = (type + 1) % ITEM_TYPE_COUNT;
		}
	}

	auto info = items[type];

	bn::fixed per_price = bn::max(bn::fixed(1), bn::fixed(info.avg_unit_price) + _rng.get_fixed(3) - bn::fixed(1));
	unsigned int amount = 10 + _rng.get_int(20);

	bounty b{
		.resource = static_cast<item_type>(type),
		.amount = amount,
		.price = (unsigned int)(bn::fixed(amount) * per_price).round_integer(),
		.collected = false
	};
	_bounties.push_back(b);
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

void shared_state::clear_collected_bounties() {
	// hack until i can figure out how to remove them properly
	for (int i = 0; i < _bounties.max_size(); i++) {
		for (auto it = _bounties.begin(); it != _bounties.end(); ++it) {
			if ((*it).collected) {
				_bounties.erase(it);
				break;
			}
		}
	}

	// repopulate
	_rng.set_seed(_frames);
	while (!_bounties.full()) {
		_generate_bounty();
	}
}

bool shared_state::upgrade_module(upgrade_type p_type) {
	auto v = get_remaining_upgrade_slot_count();
	if (v == 0) {
		return false;
	}
	auto &c = _saved_data.upgrade_levels[static_cast<int>(p_type)];
	if (c < MAX_UPGRADE_LEVEL) {
		c++;
		return true;
	}
	return false;
}

bool shared_state::downgrade_module(upgrade_type p_type) {
	auto &c = _saved_data.upgrade_levels[static_cast<int>(p_type)];
	if (c != 0) {
		c--;
		return true;
	}
	return false;
}

bool shared_state::buy_upgrade_slot() {
	auto bal = get_balance();
	if (bal < UPGRADE_PRICE) {
		return false;
	}
	if (_saved_data.upgrade_slots >= (MAX_UPGRADE_LEVEL * UPGRADE_COUNT)) {
		return false;
	}
	_saved_data.balance -= UPGRADE_PRICE;
	_saved_data.upgrade_slots++;
	return true;
}

unsigned int shared_state::get_remaining_upgrade_slot_count() {
	auto total = _saved_data.upgrade_slots;

	for (size_t i = 0; i < UPGRADE_COUNT; i++) {
		auto amount = _saved_data.upgrade_levels[i];
		if (amount >= total) {
			return 0;
		}
		total -= _saved_data.upgrade_levels[i];
	}

	return total;
}

bool shared_state::buy_rocket_launcher() {
	auto bal = get_balance();
	if (bal < ROCKET_LAUNCHER_PRICE || _saved_data.has_rocket_launcher) {
		return false;
	}
	_saved_data.balance -= ROCKET_LAUNCHER_PRICE;
	_saved_data.has_rocket_launcher = true;
	return true;
}

void shared_state::play_click() {
	if (_computer_click_sound.has_value()) {
		_computer_click_sound->stop();
		_computer_click_sound.reset();
	}
	_computer_click_sound = bn::sound_items::computer_click.play(0.4);
}

void shared_state::play_load() {
	if (_computer_load_sound.has_value()) {
		_computer_load_sound->stop();
		_computer_load_sound.reset();
	}
	_computer_load_sound = bn::sound_items::computer_load.play(1.0);
}

void shared_state::play_tick(bool p_up) {
	if (_tick_sound.has_value()) {
		_tick_sound->stop();
		_tick_sound.reset();
	}
	_tick_sound = bn::sound_items::tick.play(0.8, p_up ? bn::fixed(1.0) : bn::fixed(0.8), 0.0);
}

void shared_state::play_whoosh() {
	if (_whoosh_sound.has_value()) {
		_whoosh_sound->stop();
		_whoosh_sound.reset();
	}
	_whoosh_sound = bn::sound_items::whoosh.play(0.9);
}

void shared_state::play_deny() {
	if (_deny_sound.has_value()) {
		_deny_sound->stop();
		_deny_sound.reset();
	}
	_deny_sound = bn::sound_items::deny.play(0.8);
}

void shared_state::play_select() {
	if (_select_sound.has_value()) {
		_select_sound->stop();
		_select_sound.reset();
	}
	_select_sound = bn::sound_items::select.play(0.8);
}

} //namespace game
