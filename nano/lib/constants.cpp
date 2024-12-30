#include <nano/crypto/blake2/blake2.h>
#include <nano/lib/block_type.hpp>
#include <nano/lib/blocks.hpp>
#include <nano/lib/config.hpp>
#include <nano/lib/constants.hpp>
#include <nano/lib/env.hpp>
#include <nano/lib/logging.hpp>
#include <nano/lib/work_version.hpp>

namespace
{
// useful for boost_lexical cast to allow conversion of hex strings
template <typename ElemT>
struct HexTo
{
	ElemT value;

	HexTo () = default;

	HexTo (ElemT val) :
		value{ val }
	{
	}

	operator ElemT () const
	{
		return value;
	}

	friend std::istream & operator>> (std::istream & in, HexTo & out)
	{
		in >> std::hex >> out.value;
		return in;
	}
};
}

nano::work_thresholds const nano::work_thresholds::publish_full (
0xffffffc000000000,
0xfffffff800000000, // 8x higher than epoch_1
0xfffffe0000000000 // 8x lower than epoch_1
);

nano::work_thresholds const nano::work_thresholds::publish_beta (
0xfffff00000000000, // 64x lower than publish_full.epoch_1
0xfffff00000000000, // same as epoch_1
0xffffe00000000000 // 2x lower than epoch_1
);

nano::work_thresholds const nano::work_thresholds::publish_dev (
0xfe00000000000000, // Very low for tests
0xffc0000000000000, // 8x higher than epoch_1
0xf000000000000000 // 8x lower than epoch_1
);

nano::work_thresholds const nano::work_thresholds::publish_test ( // defaults to live network levels
nano::env::get<HexTo<uint64_t>> ("NANO_TEST_EPOCH_1").value_or (0xffffffc000000000),
nano::env::get<HexTo<uint64_t>> ("NANO_TEST_EPOCH_2").value_or (0xfffffff800000000), // 8x higher than epoch_1
nano::env::get<HexTo<uint64_t>> ("NANO_TEST_EPOCH_2_RECV").value_or (0xfffffe0000000000) // 8x lower than epoch_1
);

uint64_t nano::work_thresholds::threshold_entry (nano::work_version const version_a, nano::block_type const type_a) const
{
	uint64_t result{ std::numeric_limits<uint64_t>::max () };
	if (type_a == nano::block_type::state)
	{
		switch (version_a)
		{
			case nano::work_version::work_1:
				result = entry;
				break;
			default:
				debug_assert (false && "Invalid version specified to work_threshold_entry");
		}
	}
	else
	{
		result = epoch_1;
	}
	return result;
}

#ifndef NANO_FUZZER_TEST
uint64_t nano::work_thresholds::value (nano::root const & root_a, uint64_t work_a) const
{
	uint64_t result;
	blake2b_state hash;
	blake2b_init (&hash, sizeof (result));
	blake2b_update (&hash, reinterpret_cast<uint8_t *> (&work_a), sizeof (work_a));
	blake2b_update (&hash, root_a.bytes.data (), root_a.bytes.size ());
	blake2b_final (&hash, reinterpret_cast<uint8_t *> (&result), sizeof (result));
	return result;
}
#else
uint64_t nano::work_thresholds::value (nano::root const & root_a, uint64_t work_a) const
{
	return base + 1;
}
#endif

uint64_t nano::work_thresholds::threshold (nano::block_details const & details_a) const
{
	static_assert (nano::epoch::max == nano::epoch::epoch_2, "work_v1::threshold is ill-defined");

	uint64_t result{ std::numeric_limits<uint64_t>::max () };
	switch (details_a.epoch)
	{
		case nano::epoch::epoch_2:
			result = (details_a.is_receive || details_a.is_epoch) ? epoch_2_receive : epoch_2;
			break;
		case nano::epoch::epoch_1:
		case nano::epoch::epoch_0:
			result = epoch_1;
			break;
		default:
			debug_assert (false && "Invalid epoch specified to work_v1 ledger work_threshold");
	}
	return result;
}

uint64_t nano::work_thresholds::threshold (nano::work_version const version_a, nano::block_details const details_a) const
{
	uint64_t result{ std::numeric_limits<uint64_t>::max () };
	switch (version_a)
	{
		case nano::work_version::work_1:
			result = threshold (details_a);
			break;
		default:
			debug_assert (false && "Invalid version specified to ledger work_threshold");
	}
	return result;
}

double nano::work_thresholds::normalized_multiplier (double const multiplier_a, uint64_t const threshold_a) const
{
	debug_assert (multiplier_a >= 1);
	auto multiplier (multiplier_a);
	/* Normalization rules
	ratio = multiplier of max work threshold (send epoch 2) from given threshold
	i.e. max = 0xfe00000000000000, given = 0xf000000000000000, ratio = 8.0
	normalized = (multiplier + (ratio - 1)) / ratio;
	Epoch 1
	multiplier	 | normalized
	1.0 		 | 1.0
	9.0 		 | 2.0
	25.0 		 | 4.0
	Epoch 2 (receive / epoch subtypes)
	multiplier	 | normalized
	1.0 		 | 1.0
	65.0 		 | 2.0
	241.0 		 | 4.0
	*/
	if (threshold_a == epoch_1 || threshold_a == epoch_2_receive)
	{
		auto ratio (nano::difficulty::to_multiplier (epoch_2, threshold_a));
		debug_assert (ratio >= 1);
		multiplier = (multiplier + (ratio - 1.0)) / ratio;
		debug_assert (multiplier >= 1);
	}
	return multiplier;
}

double nano::work_thresholds::denormalized_multiplier (double const multiplier_a, uint64_t const threshold_a) const
{
	debug_assert (multiplier_a >= 1);
	auto multiplier (multiplier_a);
	if (threshold_a == epoch_1 || threshold_a == epoch_2_receive)
	{
		auto ratio (nano::difficulty::to_multiplier (epoch_2, threshold_a));
		debug_assert (ratio >= 1);
		multiplier = multiplier * ratio + 1.0 - ratio;
		debug_assert (multiplier >= 1);
	}
	return multiplier;
}

uint64_t nano::work_thresholds::threshold_base (nano::work_version const version_a) const
{
	uint64_t result{ std::numeric_limits<uint64_t>::max () };
	switch (version_a)
	{
		case nano::work_version::work_1:
			result = base;
			break;
		default:
			debug_assert (false && "Invalid version specified to work_threshold_base");
	}
	return result;
}

uint64_t nano::work_thresholds::difficulty (nano::work_version const version_a, nano::root const & root_a, uint64_t const work_a) const
{
	uint64_t result{ 0 };
	switch (version_a)
	{
		case nano::work_version::work_1:
			result = value (root_a, work_a);
			break;
		default:
			debug_assert (false && "Invalid version specified to work_difficulty");
	}
	return result;
}

uint64_t nano::work_thresholds::difficulty (nano::block const & block_a) const
{
	return difficulty (block_a.work_version (), block_a.root (), block_a.block_work ());
}

bool nano::work_thresholds::validate_entry (nano::work_version const version_a, nano::root const & root_a, uint64_t const work_a) const
{
	return difficulty (version_a, root_a, work_a) < threshold_entry (version_a, nano::block_type::state);
}

bool nano::work_thresholds::validate_entry (nano::block const & block_a) const
{
	return difficulty (block_a) < threshold_entry (block_a.work_version (), block_a.type ());
}