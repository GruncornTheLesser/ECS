#pragma once
#include "core/fwd.h"
#include <condition_variable>

namespace ecs
{
	struct priority_shared_mutex {
	public:
		void lock(priority p = priority::MEDIUM) {
			std::unique_lock lk(mtx);

			if (lock_count != 0) {
				++exclusive_queue_count[static_cast<std::size_t>(p)];  // increment wait count
				exclusive_queue[static_cast<std::size_t>(p)].wait(lk); // wait on cnd var
				--exclusive_queue_count[static_cast<std::size_t>(p)];  // de-increment wait count
			}
			lock_count = -1; // set lock_count to max
		}
		void unlock() {
			std::unique_lock lk(mtx);

			lock_count = 0; // clear lock_count
			notify();	   // notify next

		}

		void lock_shared(priority p = priority::MEDIUM) {
			std::unique_lock lk(mtx);

			if (lock_count != 0) {
				++shared_queue_count[static_cast<std::size_t>(p)]; // increment wait count
				shared_queue.wait(lk);							 // wait on cnd var
				--shared_queue_count[static_cast<std::size_t>(p)]; // de increment wait count
			}
			++lock_count; // increment lock count
		}
		void unlock_shared() {
			std::unique_lock lk(mtx);

			if (--lock_count == 0) notify(); // if all locks released, notify next
		}

	private:
		void notify() {
			for (int i = 0; i < 3; ++i) {
				if (exclusive_queue_count[i]) {	  // if thread waiting with priority
					exclusive_queue[i].notify_one(); // notify exclusive lock request
					return;
				}

				if (shared_queue_count[i]) {   // if thread waiting with priority i
					shared_queue.notify_all(); // notify all shared lock request
					return;
				}
			}
		}

		std::mutex mtx;
		std::condition_variable shared_queue;
		std::condition_variable exclusive_queue[3];
		std::size_t exclusive_queue_count[3] { 0, 0, 0 };
		std::size_t shared_queue_count[3] { 0, 0, 0 };
		std::size_t lock_count { 0 };
	};

	struct priority_mutex {
	public:
		void lock(priority p = priority::MEDIUM) {
			std::unique_lock lk(mtx);
			if (lock_count != 0) {
				++exclusive_queue_count[static_cast<std::size_t>(p)];  // increment wait count
				exclusive_queue[static_cast<std::size_t>(p)].wait(lk); // wait on cnd var
				--exclusive_queue_count[static_cast<std::size_t>(p)];  // de-increment wait count
			}
			lock_count = -1; // raise lock flag
		}
		void unlock() {
			std::unique_lock lk(mtx);
			lock_count = 0; // lower lock flag
			notify();   	// notify next
		}

	private:
		void notify() {
			for (int i = 0; i < 3; ++i) {
				if (exclusive_queue_count[i]) {	  // if thread waiting with priority i
					exclusive_queue[i].notify_one(); // notify next
					return;
				}
			}
		}

		std::mutex mtx;
		std::condition_variable exclusive_queue[3];
		std::size_t exclusive_queue_count[3]{ 0, 0, 0 };
		std::size_t lock_count { 0 };
	};
}