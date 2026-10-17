#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

namespace mc68k
{
	// A FIFO for the emulated buses' word and vector queues. Unlike std::deque, which allocates and
	// frees a block every time its front or back crosses a block boundary (even at a constant
	// depth), its storage only grows, by doubling, when the queue is deeper than it has ever been.
	// A queue that stays within its high-water mark never touches the heap, so it is safe on the
	// audio thread once warm.
	template<typename T>
	class RingQueue
	{
	public:
		explicit RingQueue(const size_t _capacity = 16)
		{
			size_t c = 1;
			while(c < _capacity)
				c <<= 1;
			m_data.resize(c);
		}

		bool empty() const { return m_size == 0; }
		size_t size() const { return m_size; }

		void push_back(const T& _v)
		{
			if(m_size == m_data.size())
				grow();
			m_data[(m_head + m_size) & mask()] = _v;
			++m_size;
		}

		const T& front() const
		{
			assert(m_size);
			return m_data[m_head];
		}

		void pop_front()
		{
			assert(m_size);
			m_head = (m_head + 1) & mask();
			--m_size;
		}

		void clear()
		{
			m_head = 0;
			m_size = 0;
		}

		// Element _i counted from the front.
		const T& operator[](const size_t _i) const { return m_data[(m_head + _i) & mask()]; }

		// Removes element _i counted from the front, keeping the order of the others.
		void erase(const size_t _i)
		{
			assert(_i < m_size);
			for(size_t k = _i; k + 1 < m_size; ++k)
				m_data[(m_head + k) & mask()] = m_data[(m_head + k + 1) & mask()];
			--m_size;
		}

	private:
		size_t mask() const { return m_data.size() - 1; }

		void grow()
		{
			std::vector<T> data(m_data.size() * 2);
			for(size_t i = 0; i < m_size; ++i)
				data[i] = (*this)[i];
			m_data.swap(data);
			m_head = 0;
		}

		std::vector<T> m_data;
		size_t m_head = 0;
		size_t m_size = 0;
	};
}
