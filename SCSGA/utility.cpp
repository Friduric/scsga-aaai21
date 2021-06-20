#define _CRT_SECURE_NO_WARNINGS

#include "utility.h"

#if defined(__GNUG__)
#include <x86intrin.h>
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

#include <vector>
#include <ctime>

uint32_t utility::bits::bit_count_32bit(uint32_t n_value)
{
#if defined(__GNUG__)
	return __builtin_popcount(n_value);
#elif defined(_MSC_VER)
	return __popcnt(n_value);
#endif
}

uint32_t utility::bits::calc_number_of_bits_set_32bit(uint32_t n_mask)
{
	uint32_t n_count = 0;
	while (n_mask > 0U)
	{
		n_count += (n_mask & 1);
		n_mask >>= 1;
	}
	return n_count;
}

uint64_t utility::bits::calc_number_of_bits_set_64bit(uint64_t n_mask)
{
	uint64_t n_count = 0;
	while (n_mask > 0ULL)
	{
		n_count += (n_mask & 1);
		n_mask >>= 1;
	}
	return n_count;
}

uint32_t utility::bits::calc_parallel_bits_deposit_32bit(uint32_t n_val, uint32_t n_mask)
{
	uint32_t n_result = 0U;
	for (uint32_t n_current_mask = 1U; n_mask; n_current_mask += n_current_mask)
	{
#pragma warning( disable : 4146 ) // Disable warning for negating unsigned integer.
		if (n_val& n_current_mask) n_result |= n_mask & -n_mask;
#pragma warning( default : 4146 )
		n_mask &= n_mask - 1U;
	}
	return n_result;
}

uint64_t utility::bits::calc_parallel_bits_deposit_64bit(uint64_t n_val, uint64_t n_mask)
{
	uint64_t n_result = 0ULL;
	for (uint64_t n_current_mask = 1ULL; n_mask; n_current_mask += n_current_mask)
	{
#pragma warning( disable : 4146 ) // Disable warning for negating unsigned integer.
		if (n_val& n_current_mask) n_result |= n_mask & -n_mask;
#pragma warning( default : 4146 )
		n_mask &= n_mask - 1ULL;
	}
	return n_result;
}

std::string utility::date_and_time::get_current_time_and_date_as_string()
{
	/* Code from https://stackoverflow.com/questions/16357999/current-date-and-time-as-string */
	time_t rawtime;
	struct tm* timeinfo;
	char buffer[160];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
	return std::string(buffer);
}

double utility::statistics::calc_mean(const std::vector<float> & values)
{
	double mean = 0;
	for (int i = 0; i < values.size(); ++i)
		mean += (values[i] - mean) / double(double(i) + 1);
	return mean;
}

double utility::statistics::calc_variance(const std::vector<float>& values)
{
	double variance = 0;
	double mean = utility::statistics::calc_mean(values);
	for (auto v : values)
	{
		variance += (v - mean) * (v - mean);
	}
	variance /= values.size();
	return variance;
}

float utility::statistics::calc_standard_deviation(const std::vector<float>& values)
{
	if (values.size() <= 1) return 0;

	/* Based on the pseudocode at https://www.radford.edu/~biol-web/stats/standarderrorcalc.pdf */

	// Step 1: Calculate the mean.
	double mean = calc_mean(values);

	// Step 2: Calculate each measurement's deviation from the mean.
	std::vector<double> deviation_from_mean(values.size());
	for (int i = 0; i < values.size(); ++i)
		deviation_from_mean[i] = double(mean) - double(values[i]);

	// Step 3: Square each deviation from mean.
	for (int i = 0; i < values.size(); ++i)
		deviation_from_mean[i] = deviation_from_mean[i] * deviation_from_mean[i];

	// Step 4: Sum the squared deviations (beware of overflow!).
	double sum = 0.0;
	for (int i = 0; i < values.size(); ++i)
		sum += deviation_from_mean[i];

	// Step 5: Divide the sum by one less than the sample size.
	sum /= double(values.size() - 1);

	// Step 6: Calculate standard deviation.
	return float(sqrt(sum));
}

float utility::statistics::calc_standard_error(const std::vector<float>& values)
{
	if (values.size() <= 1) return 0;

	/* Based on the pseudocode at https://www.radford.edu/~biol-web/stats/standarderrorcalc.pdf */
	return calc_standard_deviation(values) / float(values.size());
}

void utility::date_and_time::timer::start()
{
	assert(!running);

	start_time = clock_t::now();
	running = true;
}

float utility::date_and_time::timer::stop()
{
	assert(running);

	float time{ pause() };
	reset();
	return time;
}

float utility::date_and_time::timer::pause()
{
	assert(running);

	elapsed_time += get_time();
	running = false;
	return elapsed_time;
}

void utility::date_and_time::timer::reset()
{
	elapsed_time = 0;
	running = false;
	countdown = -1;
}

float utility::date_and_time::timer::get_time() const
{
	time_t end_time = clock_t::now();
	if (running) {
		return elapsed_time + std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
	} else {
		return elapsed_time;
	}
}

void utility::date_and_time::timer::start_countdown(const float time) {
	elapsed_time = 0;
	start_time = clock_t::now();
	countdown = time;
	running = true;
}

bool utility::date_and_time::timer::countdown_reached() const {
	return countdown >= 0 && get_time() > countdown;
}