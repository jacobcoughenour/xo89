#pragma once

#include "items.h"
#include "state/state.h"

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

using bounty_list = bn::vector<bounty, 8>;

struct save_data {
	unsigned int format = 1;
	int balance = 0;
	unsigned int ship_inventory[ITEM_TYPE_COUNT];
};

class shared_state : public state {
public:
	explicit shared_state();
	void update();
	void new_game();

	void generate_bounties();

	unsigned int get_frame_count();
	unsigned int get_inventory_count(item_type p_item_type);
	void deposit_to_inventory(item_type p_item_type, unsigned int p_amount);
	int get_balance();
	const bounty_list &get_bounties();
	bool collect_bounty(int p_bounty_index);

	void load();
	void save();

private:
	bn::seed_random _rng;
	unsigned int _frames = 0;
	bounty_list _bounties;

	save_data _saved_data;

	void _withdraw_from_inventory(item_type p_item_type, unsigned int p_amount);
};
} //namespace game