#include "ringQueue.h"

#include <cstdint>
#include <deque>
#include <iostream>
#include <random>

// RingQueue replaces std::deque for the HDI08 receive and interrupt queues, so it must behave as
// one for every operation those use: FIFO order across wrap-around and growth, size, clear,
// indexed reads from the front, and erase in the middle. Checked against std::deque on a long
// random sequence that starts small (capacity 4), so growth from a wrapped state is exercised.
int main()
{
	mc68k::RingQueue<uint32_t> ring(4);
	std::deque<uint32_t> ref;
	std::mt19937 rng(12345);
	uint32_t next = 0;
	int failures = 0;

	auto fail = [&](const char* _what, const size_t _step)
	{
		if(failures++ < 10)
			std::cerr << "ring_queue_test: " << _what << " at step " << _step << '\n';
	};

	for(size_t step = 0; step < 200000; ++step)
	{
		const auto op = rng() % 100;
		// Bias depth upwards over time, then back down, so the ring grows several times.
		const bool pushBias = (step / 20000) % 2 == 0;
		if(op < (pushBias ? 55u : 40u))
		{
			ring.push_back(next);
			ref.push_back(next);
			++next;
		}
		else if(op < 95)
		{
			if(!ref.empty())
			{
				if(ring.front() != ref.front())
					fail("front", step);
				ring.pop_front();
				ref.pop_front();
			}
		}
		else if(op < 99)
		{
			if(!ref.empty())
			{
				const size_t i = rng() % ref.size();
				ring.erase(i);
				ref.erase(ref.begin() + static_cast<std::ptrdiff_t>(i));
			}
		}
		else if(rng() % 50 == 0)
		{
			ring.clear();
			ref.clear();
		}

		if(ring.size() != ref.size() || ring.empty() != ref.empty())
			fail("size", step);
		if(step % 997 == 0)
			for(size_t i = 0; i < ref.size(); ++i)
				if(ring[i] != ref[i])
				{
					fail("element", step);
					break;
				}
	}

	if(failures)
	{
		std::cerr << "ring_queue_test: " << failures << " failures\n";
		return 1;
	}
	std::cout << "ring_queue_test: ok\n";
	return 0;
}
