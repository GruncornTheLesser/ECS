#pragma once
#include "core/fwd.h"
#include <condition_variable>

namespace ecs
{
	struct null_mutex {
		void lock(priority p = priority::MEDIUM) { }
		void unlock() { }
	};

	struct priority_shared_mutex
	{
	public:
		void lock(priority p = priority::MEDIUM)
		{
			std::unique_lock lk(mtx);
			if (current != 0)
			{
				++exclusive_queue_count[static_cast<std::size_t>(p)];
				exclusive_queue[static_cast<std::size_t>(p)].wait(lk);
				--exclusive_queue_count[static_cast<std::size_t>(p)];
			}
			current = -1;
		}
		void unlock()
		{
			std::unique_lock lk(mtx);
			current = 0;
			notify_next();

		}

		void lock_shared(priority p = priority::MEDIUM)
		{
			std::unique_lock lk(mtx);
			if (current != 0)
			{
				++shared_queue_count[static_cast<std::size_t>(p)];
				shared_queue.wait(lk);
				--shared_queue_count[static_cast<std::size_t>(p)];
			}
			++current;
		}
		void unlock_shared()
		{
			std::unique_lock lk(mtx);
			--current;
			if (current == 0) notify_next();
		}

	private:
		void notify_next()
		{
			for (int i = 0; i < 3; ++i)
			{
				if (exclusive_queue_count[i])
				{
					exclusive_queue[i].notify_one();
					return;
				}

				if (shared_queue_count[i])
				{
					shared_queue.notify_all();
					return;
				}
			}
		}

		std::mutex mtx;
		std::condition_variable shared_queue;
		std::condition_variable exclusive_queue[3];
		std::size_t exclusive_queue_count[3]{ 0,0,0 };
		std::size_t shared_queue_count[3]{ 0,0,0 };
		std::size_t current { 0 };
	};

	struct priority_mutex
	{
	public:
		void lock(priority p = priority::MEDIUM)
		{
			std::unique_lock lk(mtx);
			if (current != 0)
			{
				++exclusive_queue_count[static_cast<std::size_t>(p)];
				exclusive_queue[static_cast<std::size_t>(p)].wait(lk);
				--exclusive_queue_count[static_cast<std::size_t>(p)];
			}
			current = -1;
		}
		void unlock()
		{
			std::unique_lock lk(mtx);
			current = 0;
			notify_next();

		}

	private:
		void notify_next()
		{
			for (int i = 0; i < 3; ++i)
			{
				if (exclusive_queue_count[i])
				{
					exclusive_queue[i].notify_one();
					return;
				}
			}
		}

		std::mutex mtx;
		std::condition_variable exclusive_queue[3];
		std::size_t exclusive_queue_count[3]{ 0,0,0 };
		std::size_t current { 0 };
	};
}