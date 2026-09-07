#pragma once

#include "items.h"
#include "state/state.h"
#include "upgrades.h"

#include "bn_assert.h"
#include "bn_list.h"
#include "bn_log.h"
#include "bn_seed_random.h"
#include "bn_sram.h"
#include "bn_vector.h"

namespace game {

struct bounty {
	item_type resource;
	unsigned int amount;
	// todo what if you have to pay someone to get rid of items?
	unsigned int price;
	bool collected;
};

using bounty_list = bn::vector<bounty, 5>;

struct save_data {
	unsigned int format = 1;
	int balance = 0;
	unsigned int ship_inventory[ITEM_TYPE_COUNT];
	unsigned int upgrade_levels[UPGRADE_COUNT];
	unsigned int upgrade_slots = 0;
	bool has_rocket_launcher = false;
};

class shared_state : public state {
public:
	explicit shared_state();
	void update();
	void new_game();
	void ensure_loaded();

	void generate_bounties();

	unsigned int get_frame_count();
	unsigned int get_inventory_count(item_type p_item_type);
	void deposit_to_inventory(item_type p_item_type, unsigned int p_amount);
	int get_balance();
	const bounty_list &get_bounties();
	bool collect_bounty(int p_bounty_index);
	void clear_collected_bounties();

	void upgrade_module(upgrade_type p_type);
	void downgrade_module(upgrade_type p_type);
	unsigned int get_upgrade_level(upgrade_type p_type) { return bn::min((unsigned int)MAX_UPGRADE_LEVEL, _saved_data.upgrade_levels[static_cast<int>(p_type)]); }
	void buy_upgrade_slot();
	unsigned int get_total_upgrade_slot_count() { return _saved_data.upgrade_slots; }
	unsigned int get_remaining_upgrade_slot_count();

	void buy_rocket_launcher();
	bool get_has_rocket_launcher() { return _saved_data.has_rocket_launcher; }

	int get_mining_duration() { return 90 - (get_upgrade_level(upgrade_type::MINING_SPEED) * 25); }
	int get_fire_cooldown() { return 25 - (get_upgrade_level(upgrade_type::FIRE_RATE) * 5); }

	bool has_save();
	void load();
	void save();

	bn::seed_random audio_rng;

private:
	bool _loaded;
	bn::seed_random _rng;
	unsigned int _frames = 0;
	bounty_list _bounties;

	save_data _saved_data;

	void _withdraw_from_inventory(item_type p_item_type, unsigned int p_amount);

	void _generate_bounty();
};

} //namespace game