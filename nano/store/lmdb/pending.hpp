#pragma once

#include <nano/store/pending.hpp>

#include <lmdb/libraries/liblmdb/lmdb.h>

namespace nano::store::lmdb
{
class component;
}
namespace nano::store::lmdb
{
class pending : public nano::store::pending
{
private:
	nano::store::lmdb::component & store;

public:
	explicit pending (nano::store::lmdb::component & store_a);
	void put (store::write_transaction const & transaction_a, nano::pending_key const & key_a, nano::pending_info const & pending_info_a) override;
	void del (store::write_transaction const & transaction_a, nano::pending_key const & key_a) override;
	std::optional<nano::pending_info> get (store::transaction const & transaction_a, nano::pending_key const & key_a) override;
	bool exists (store::transaction const & transaction_a, nano::pending_key const & key_a) override;
	bool any (store::transaction const & transaction_a, nano::account const & account_a) override;
	iterator begin (store::transaction const & transaction_a, nano::pending_key const & key_a) const override;
	iterator begin (store::transaction const & transaction_a) const override;
	iterator end (store::transaction const & transaction_a) const override;
	void for_each_par (std::function<void (store::read_transaction const &, iterator, iterator)> const & action_a) const override;

	/**
	 * Maps min_version 0 (destination account, pending block) to (source account, amount). (Removed)
	 * nano::account, nano::block_hash -> nano::account, nano::amount
	 */
	MDB_dbi pending_v0_handle{ 0 };

	/**
	 * Maps min_version 1 (destination account, pending block) to (source account, amount). (Removed)
	 * nano::account, nano::block_hash -> nano::account, nano::amount
	 */
	MDB_dbi pending_v1_handle{ 0 };

	/**
	 * Maps (destination account, pending block) to (source account, amount, version). (Removed)
	 * nano::account, nano::block_hash -> nano::account, nano::amount, nano::epoch
	 */
	MDB_dbi pending_handle{ 0 };
};
} // namespace nano::store::lmdb
