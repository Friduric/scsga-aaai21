#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cmath>

#include "coalition.h"

namespace utility
{
	namespace combinatorics
	{
		namespace
		{
			static void gen_all_integer_partitions_of_size_rec
			(
				const int n_size, const int n_max, const int n_addends,
				std::vector<std::vector<int>>& result_partitions, std::vector<int> current_partition
			)
			{
				if (current_partition.size() > n_addends) return;
				if (n_size == 0)
				{
					result_partitions.push_back(current_partition);
					return;
				}
				for (int i = std::min<int>(n_size, n_max); i >= 1; --i)
				{
					current_partition.push_back(i);
					gen_all_integer_partitions_of_size_rec(n_size - i, i, n_addends, result_partitions, current_partition);
					current_partition.pop_back();
				}
			}
		}

		// Generates all possible fixed-size integer partitions of a given number.
		//
		// An integer partition of a number is "a way" of writing that number as a set of 
		// addends. For example, 4 = 2 + 1 + 1, which means that {2, 1, 1} is an integer partition
		// of 4. Also, 4 = 3 + 1, so {3, 1} is also an integer partition of 4. 
		//
		// This function generates all such integer partitions of a fixed-size (given by nAddends).
		// For example, if nNumber = 7 and nAddends = 3, calling this function would generate:
		// {{5,1,1}, {4,2,1}, {3,3,1}, {3,2,2}, {4,3,0}, {5,2,0}, {6,1,0}, {7,0,0}, ... }
		static std::vector<std::vector<int>> gen_all_integer_partitions_of_size(int n_number, int n_addends)
		{
			std::vector<std::vector<int>> Subsets;
			std::vector<int> EmptyPartition;
			EmptyPartition.reserve(n_addends);
			gen_all_integer_partitions_of_size_rec(n_number, n_number, n_addends, Subsets, EmptyPartition);
			for (auto& Subset : Subsets)
			{
				while (Subset.size() < n_addends)
				{
					Subset.push_back(0);
				}
			}
			return Subsets;
		}
	}

	namespace bits
	{
		uint32_t bit_count_32bit(uint32_t n_value);

		uint32_t calc_number_of_bits_set_32bit(uint32_t n_mask);
		uint64_t calc_number_of_bits_set_64bit(uint64_t n_mask);

		// *************************************************************************
		// 32 bit PDEP (parallel bits deposit). 
		// See e.g. https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets
		// *************************************************************************
		uint32_t calc_parallel_bits_deposit_32bit(uint32_t n_val, uint32_t n_mask);

		// *************************************************************************
		// 64 bit PDEP (parallel bits deposit). 
		// See e.g. https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets
		// *************************************************************************
		uint64_t calc_parallel_bits_deposit_64bit(uint64_t n_val, uint64_t n_mask);

		// A simple wrapper for a 64bit mask.
		struct SMask
		{
			uint64_t _nMask = 0ULL;
			inline void AddToMask(uint64_t nIndex) { _nMask |= (1ULL << nIndex); }
			inline void RemoveFromMask(uint64_t nIndex) { _nMask &= ~(1ULL << nIndex); }
		};
	}

	namespace statistics
	{
		double calc_mean(const std::vector<float>& values);
		double calc_variance(const std::vector<float>& values);
		float calc_standard_deviation(const std::vector<float>& values);
		float calc_standard_error(const std::vector<float>& values);
	}

	namespace date_and_time
	{
		std::string get_current_time_and_date_as_string();

		/*
			Simple utility class to track elapsed time.
		*/
		class timer {
		public:

			/*
				Starts/restarts the timer.
			*/
			void start();

			/*
				Stops the timer and resets its internal state. Equivalent
				to pause() followed by reset(). Returns the total time
				elapsed in seconds while the timer was active.
			*/
			float stop();

			/*
				Pauses the timer without resetting the internal state.
				If the timer is resumed later, the time elapsed so far is
				included. Returns the total time elapsed in seconds while
				the timer has been active.
			*/
			float pause();

			/*
				Resets the internal state.
			*/
			void reset();

			/*
				Returns the total time elapsed in seconds while the timer
				has been active.
			*/
			float get_time() const;

			/*
				Start a time countdown. Use countdown_reached to check if
				countdown has been reached. To pause/restart the countdown,
				use start and pause. Negative time means unlimited countdown.
			*/
			void start_countdown(const float time);

			/*
				Check if the countdown has been reached. Returns false if no
				countdown has been started or the countdown time was negative.
			*/
			bool countdown_reached() const;

		private:
			using clock_t = std::chrono::high_resolution_clock;
			using time_t = clock_t::time_point;

			time_t start_time{};
			bool running{ false };
			float elapsed_time{ 0 };
			float countdown{ -1 };
		};
	}
}
