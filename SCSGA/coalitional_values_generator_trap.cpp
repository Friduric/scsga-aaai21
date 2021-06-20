#include "coalitional_values_generator_trap.h"

void coalitional_values_generator_trap::reset(uint32_t n_agents, uint32_t n_tasks, int seed)
{
	if (seed >= 0)
	{
		generator.seed(seed); // Used to generate samples.
	}

	task_trap_generators.resize(n_tasks);
	for (uint32_t t = 0; t < n_tasks; ++t)
	{
		for (uint32_t i = 0; i <= n_agents; ++i)
		{
			/*
				f(x) = a + bx + cx ^ 2
				_________________
				f(0) = L
				f(n / 2) = 0
				f(n) = H
				_________________
				=>
				a = L
				b = -2L / n - (H + L) / n
				c = 2(H + l) / n^2
				_________________
				*/
			float L = 0.01;
			float H = 10.0;

			float n = n_agents;
			float a = L;
			float b = (-2 * L / n) - (H + L) / n;
			float c = 2 * (H + L) / (n * n);

			float k = a + b * i + c * i * i; // Calculated according to formula above (derived on black board).

			trap_generators.push_back(std::normal_distribution<coalition::value_t>(i <= 1 ? L : k, 0.1f));
			values.push_back(i <= 1 ? L : k);

			task_trap_generators[t].push_back(std::normal_distribution<coalition::value_t>(t * (i <= 1 ? L : k), 0.1f));
		}
	}
}

coalition::value_t coalitional_values_generator_trap::generate_new_value(
	const coalition::coalition_t& coalition,
	const uint32_t task
)
{
	coalition::value_t value{};
	switch (MODE)
	{
	case 0:
		value = values[coalition.count_agents_in_coalition()];
		break;
	case 1:
	{
		uint32_t x = coalition.count_agents_in_coalition();

		float xf = x;

		float cost = -xf * xf;
		float steep = pow(xf, 2.1f);
		float val = xf + (xf > n_agents / 2.0f ? steep : 0.0f);

		float mean = cost + val;
		float trap_value =
			std::normal_distribution<float>(0.1 * mean, 0.1f)(generator);

		value = trap_value;
	}
	break;
	case 2:
		value = task_trap_generators[task][coalition.count_agents_in_coalition()](generator);
		break;
	default:
		break;
	}
	return value;
}

std::string coalitional_values_generator_trap::get_file_name() const
{
	return "trap_" + std::to_string(seed) + "_" + std::to_string(n_agents) + "_" + std::to_string(n_tasks) + ".problem";
}