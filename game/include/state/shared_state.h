#pragma once

#include "obj_type.h"
#include "state/state.h"

#include "bn_assert.h"
#include "bn_list.h"
#include "bn_seed_random.h"
#include "bn_vector.h"

namespace game {

struct bounty {
	obj_type resource;
	unsigned int amount;
	// todo what if you have to pay someone to get rid of items?
	unsigned int price;
	bool collected;
};

using bounty_list = bn::vector<bounty, 8>;

class shared_state : public state {
public:
	explicit shared_state();
	void update();

	void generate_bounties();

	unsigned int get_frame_count();
	int get_inventory_count(obj_type p_obj_type);
	void deposit_to_inventory(obj_type p_obj_type, unsigned int p_amount);
	int get_balance();
	const bounty_list &get_bounties();
	bool collect_bounty(int p_bounty_index);

	// todo do save load here

private:
	bn::seed_random _rng;
	unsigned int _frames = 0;
	int _ship_inventory[ITEM_TYPE_COUNT];
	int _balance = 0;
	bounty_list _bounties;

	void _withdraw_from_inventory(obj_type p_obj_type, unsigned int p_amount);
};
} //namespace game