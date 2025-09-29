#include <algorithm>
#include <ranges>
#include <vector>

namespace ecs::policy {
	struct optimal {
		constexpr auto emplace(auto& range, auto it, auto&& ... args) {
			if (it == range.end()) {
				range.emplace_back(std::forward<decltype(args)>(args)...);
				return std::ranges::subrange(range.end(), range.end());
			} else {
				range.emplace_back(std::move(*it));
				std::construct_at(&*it, std::forward<decltype(args)>(args)...);
				return std::ranges::subrange(range.end() - 1, range.end());
			}

		}

		constexpr auto emplace_n(auto& range, auto it, std::size_t n, auto&& ... args) {
			auto end = range.end();
			auto dst = it + n;
			
			for (; dst > end; --dst) {
				range.emplace_back(std::forward<decltype(args)>(args)...);
			}
			
			auto mv = range.end();
			std::move(it, dst, std::back_inserter(range));
			auto mv_end = range.end();

			--dst;
			for (; dst >= it; --dst) {
				std::construct_at(&(*dst), std::forward<decltype(args)>(args)...);
			}
			
			return std::ranges::subrange(mv, mv_end);
		}

		constexpr auto insert_range(auto& range, auto it, auto&& input_range) {
			std::size_t n = std::size(input_range);

			auto end = range.end();
			auto dst = it + n;
			auto src = input_range.end();
		
			if (end < dst) {
				src -= std::distance(end, dst);
				auto fwd_src = src; 
				do { range.emplace_back(*fwd_src++); } while (--dst > end);
			}

			auto mv = range.end();
			std::move(it, dst, std::back_inserter(range));
			auto mv_end = range.end();

			--dst;
			--src;
			for (; dst >= it; --dst, --src) { 
				*dst = std::move(*src); 
			}

			return std::ranges::subrange(mv, mv_end);
		}

		constexpr auto erase(auto& range, auto it) {
			auto last = range.end() - 1;
			if (it != last) {
				(*it) = std::move(*last);
			}
			range.resize(range.size() - 1);

			return std::ranges::subrange(range.end(), range.end());
		}

		constexpr auto erase_n(auto& range, auto it, std::size_t n) {
			auto end = it + n;
			std::size_t count = std::distance(it, end);
			if (end != range.end()) {
				std::move(std::max(range.end() - count, end), range.end(), it);
			} 
			range.resize(range.size() - count);

			return std::ranges::subrange(range.end(), range.end());

		}
	};

	struct strict {
		constexpr auto emplace(auto& range, auto it, auto&& ... args) {
			range.emplace(it, std::forward<decltype(args)>(args)...);
			
			return std::ranges::subrange(it + 1, range.end());
		}

		constexpr auto emplace_n(auto& range, auto it, std::size_t n, auto&& ... args) {
			auto dst = it + n;
			auto end = range.end();
			
			if (dst > end) {
				std::size_t cnt = n - std::distance(end, dst);
				
				do { range.emplace_back(std::forward<decltype(args)>(args)...); } while (--dst > end);
				
				std::move(it, it + cnt, std::back_inserter(range));
			} else {
				std::move(dst, end, std::back_inserter(range));
			
				std::move_backward(it, dst, end);
			}

			--dst;
			for (; dst >= it; --dst) {
				std::construct_at(&(*dst), std::forward<decltype(args)>(args)...);
			}

			return std::ranges::subrange(it + n, range.end());
		}

		constexpr auto insert_range(auto& range, auto it, auto&& input_range) {
			std::size_t n = std::size(input_range);
			range.insert_range(it, input_range);
			return std::ranges::subrange(it + n, range.end());
		}

		constexpr auto erase(auto& range, auto it) {
			range.erase(it);
			return std::ranges::subrange(it, range.end());
		}

		constexpr auto erase_n(auto& range, auto it, std::size_t n) {
			range.erase(it, it + n);
			return std::ranges::subrange(it, range.end());
		}
	};
}