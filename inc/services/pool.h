#pragma once
#include "core/traits.h"
#include <cassert>
#include <span>
#include <algorithm>

namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
	private:
		using component_type = std::remove_const_t<T>;
		using registry_type = reg_T;

		using entity_type = traits::component::get_entity_t<component_type>;
		using handle_type = traits::component::get_handle_t<component_type>;
		using value_type = util::copy_const_t<traits::component::get_value_t<component_type>, T>;

		using initialize_event = traits::component::get_initialize_event_t<component_type>;
		using terminate_event = traits::component::get_terminate_event_t<component_type>;
		
		using manager_type = util::copy_const_t<traits::component::get_manager_t<component_type>, T>;
		using indexer_type = util::copy_const_t<traits::component::get_indexer_t<component_type>, T>;
		using storage_type = util::copy_const_t<traits::component::get_storage_t<component_type>, T>;

		static constexpr bool initialize_event_enabled = !std::is_void_v<traits::component::get_initialize_event_t<T>>;
		static constexpr bool terminate_event_enabled = !std::is_void_v<traits::component::get_terminate_event_t<T>>;
		
		static constexpr bool manager_enabled = !std::is_void_v<manager_type>;
		static constexpr bool indexer_enabled = !std::is_void_v<indexer_type>;
		static constexpr bool storage_enabled = !std::is_void_v<storage_type>;

		static_assert(!manager_enabled || !indexer_enabled || !std::is_same_v<manager_type, indexer_type>);
		static_assert(!indexer_enabled || !storage_enabled || !std::is_same_v<indexer_type, storage_type>);
		static_assert(!storage_enabled || !manager_enabled || !std::is_same_v<storage_type, manager_type>);
		

		static_assert(manager_enabled);
		static_assert(indexer_enabled);
		// static_assert(storage_enabled);

		//? using iterator = view_iterator<select<entity_type, T>, from<T>, where<>, reg_T>;
		//? using const_iterator = view_iterator<select<entity_type, const T>, from<T>, where<>, const reg_T>;
		//? using sentinel = view_sentinel;

	public:
		inline constexpr pool(reg_T& reg) noexcept : reg(reg) { }

		/** the number of active + inactive components */
		[[nodiscard]] constexpr std::size_t size() const {
			if constexpr (indexer_enabled) {
				const auto& indexer = reg.template get_attribute<const indexer_type>();
				return indexer.size();
			}
		}

		[[nodiscard]] constexpr std::size_t reserved() const {
			if constexpr (manager_enabled) {
				const auto& manager = reg.template get_attribute<const manager_type>();
				return manager.size();
			}
		}

		/** returns the entity handle at the back of the pool */
		[[nodiscard]] inline constexpr const handle_type& back() const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.back();
		}

		/** returns the entity handle at the front of the pool */
		[[nodiscard]] inline constexpr const handle_type& front(std::size_t idx) const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.back();
		}
		/** returns the entity handle at the given index */
		[[nodiscard]] inline constexpr const handle_type& at(std::size_t idx) const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] inline constexpr value_type& component_at(std::size_t idx) 
			requires (storage_enabled) {
			auto& storage = reg.template get_attribute<storage_type>();
			return storage.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] inline constexpr const value_type& component_at(std::size_t idx) const requires (storage_enabled) {
			const auto& storage = reg.template get_attribute<const storage_type>();
			return storage.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] inline constexpr decltype(auto) get_component(const handle_type& hnd) {
			if constexpr (storage_enabled) {
				return component_at(index_of(hnd));
			} else {
				return contains(hnd);
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] inline constexpr decltype(auto) get_component(const handle_type& hnd) const {
			if constexpr (storage_enabled) {
				return component_at(index_of(hnd));
			} else {
				return contains(hnd);
			}
		}

		/** returns true if entity exists within the pool */
		[[nodiscard]] inline constexpr bool contains(const handle_type& hnd) const requires (indexer_enabled) {
			const auto& indexer = reg.template get_attribute<const indexer_type>();
			return indexer.contains(hnd);
		}

		/** returns the index of the component of a given entity */
		[[nodiscard]] inline constexpr std::size_t index_of(const handle_type& hnd) const requires (indexer_enabled) {
			const auto& indexer = reg.template get_attribute<const indexer_type>();
			if (auto it = indexer.find(hnd); it != indexer.end()) {
				return it->second;
			}
			
			return -1;
		}

		/** adds a component to the pool at the index */
		template<typename seq_T=policy::optimal, typename exec_T=ecs::policy::immediate, typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr decltype(auto) emplace_at(std::size_t idx, handle_type hnd, arg_Ts&&... args) {
			return _emplace_at(seq_T{}, exec_T{}, idx, std::span<handle_type>{ &hnd, 1 }, std::forward<arg_Ts>(args)...);
		}

		template<typename seq_T=policy::optimal, typename exec_T=policy::immediate, typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void emplace_at(std::size_t idx, std::span<handle_type> hnds, arg_Ts&&... args) {
			return _emplace_at(seq_T{}, exec_T{}, idx, hnds, std::forward<arg_Ts>(args)...);
		}

		/** adds a component to the back of the pool */
		template<typename seq_T=policy::optimal, typename exec_T=policy::immediate, typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr decltype(auto) emplace_back(handle_type hnd, arg_Ts&&... args) {
			return _emplace_back(policy::immediate{}, std::span<handle_type>{ &hnd, 1 }, std::forward<arg_Ts>(args)...);
		}

		template<typename seq_T=policy::optimal, typename exec_T=policy::immediate, typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void emplace_back(std::span<handle_type> hnds, arg_Ts&&... args) {
			return _emplace_back(seq_T{}, exec_T{}, hnds, std::forward<arg_Ts>(args)...);
		}
		
		template<typename seq_T=policy::optimal>
		constexpr void erase(handle_type hnd) {
			_erase_at(seq_T{}, policy::immediate{}, index_of(hnd));
		}

		template<typename seq_T=policy::optimal>
		constexpr void erase(std::span<handle_type> hnds) {
			for (auto hnd : hnds) _erase_at(seq_T{}, policy::immediate{}, index_of(hnd));
		}

		template<typename seq_T=policy::optimal, typename exec_T=policy::immediate>
		constexpr void erase_at(std::size_t idx, std::size_t n = 1) {
			return _erase_at(seq_T{}, exec_T{}, idx, n);
		}

		template<typename seq_T=policy::optimal, typename exec_T=policy::immediate>
		constexpr void clear() {
			_clear(exec_T{});
		}
		
		void sync() {
			auto& manager = reg.template get_attribute<manager_type>();
			auto& indexer = reg.template get_attribute<indexer_type>();
			
			auto begin = manager.begin() + indexer.size();
			auto end = manager.end(); //? std::remove_if(begin, manager.end(), [&](auto& hnd) { return hnd == tombstone{}; });

			if constexpr (terminate_event_enabled) {
				for (auto it = end; it < manager.end(); ++it) {
					if constexpr (storage_enabled) {
						reg.template on<terminate_event>().invoke(*it, get_component(*it));
					} else {
						reg.template on<terminate_event>().invoke(*it);
					}

					indexer.erase(*it);
				}
			}

			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				
				std::size_t size = std::distance(manager.begin(), end);
				if (size > storage.size()) {
					storage.resize(size);
				}
			}

			// cycle following algorithm
			for (auto it = begin; it < end; ++it) { 
				handle_type hnd = *it;
				std::size_t curr = std::distance(manager.begin(), it);

				// find where curr previously stored
				std::size_t next;
				if (auto it = indexer.find(hnd); it != indexer.end()) {
					next = *it;
				} else {
					continue;
				}

				while (curr != next) {
					// swap curr to the updated position
					if constexpr (storage_enabled) {
						auto& storage = reg.template get_attribute<storage_type>();
						std::swap(storage.at(curr), storage.at(next));
					}

					// update index of curr
					indexer.at(hnd) = curr;
					
					// iterate to next
					curr = next; 
					hnd = manager.at(curr);

					// find where curr previously stored
					if (auto it = indexer.find(hnd); it != indexer.end()) {
						next = *it;
					} else {
						break;
					}
				}
			}
			
			/*
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				// erase extra
				std::size_t size = std::distance(manager.begin(), end);
				if (size < storage.size()) {
					storage.resize(size);
				}
				// initialize
				if constexpr (initialize_event_enabled) {
					for (auto it = begin; it < end; ++it) {
						auto& val = storage.at(std::distance(manager.begin(), it));
						std::construct_at(&val);
						reg.template on<initialize_event>().invoke(*it, val);
					}
				}
			}

			manager.erase(end, manager.end());
			*/
		}

	private:
		// immediate policies

		template<typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void _emplace_back(ecs::policy::immediate, std::span<handle_type> hnds, arg_Ts&& ... args) {
			auto& manager = reg.template get_attribute<manager_type>();
			auto& indexer = reg.template get_attribute<indexer_type>();

			{ // indexer
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					indexer.emplace(hnds.at(i), indexer.size());
				}
			}
			
			{ // manager
				manager.reserve(manager.size() + hnds.size());
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					manager.emplace_back(hnds.at(i));
				}
			}
			
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();

				storage.reserve(storage.size() + hnds);
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					storage.emplace_back(std::forward<arg_Ts>(args)...);
				}

				if constexpr (initialize_event_enabled) {
					auto it = storage.end() - hnds.size();
					for (std::size_t i = 0; i < hnds.size(); ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i), *it++);
					}
				}
			} else {
				if constexpr (initialize_event_enabled) {
					for (std::size_t i = 0; i < hnds.size(); ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i));
					}
				}
			}
		}

		template<typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void _emplace_at(policy::optimal, policy::immediate, std::size_t idx, std::span<handle_type> hnds, arg_Ts&&... args) {
			/*
			splits new elements into elements pushed_back and elements inserted
			first pushes all elements beyond the bounds of the current range to
			the back. then swaps old elements currently within range to back.
			remaining new elements constructed inplace.
			*/

			std::size_t old_size = size();										// size of pool before emplace
			std::size_t count = hnds.size();									// number of elements to emplace
			std::size_t back_count = std::max<int>(0, idx + count - old_size);	// the number of elements pushed to back
			std::size_t inplace_count = count - back_count;						// the number of new elements constructed inplace
			std::size_t split = old_size - inplace_count;						// the index of the split between elements
			
			if constexpr (indexer_enabled) {
				auto& indexer = reg.template get_attribute<indexer_type>();
				for (std::size_t i = 0; i < count; ++i) {
					indexer.emplace(hnds.at(i), i);
				}
				
				for (std::size_t i = 0; i < count; ++i) {
					indexer.at(at(i)) = count + i;
				}
			}

			if constexpr (manager_enabled) {
				auto& manager = reg.template get_attribute<manager_type>();
				manager.reserve(old_size + count);
		
				for (std::size_t i = 0; i < back_count; ++i) {
					manager.emplace_back(hnds.at(i + inplace_count));
				}
				
				std::move(manager.begin() + split, manager.begin() + idx + inplace_count, std::back_inserter(manager));

				for (std::size_t i = 0; i < inplace_count; ++i) {
					std::construct_at(&manager.at(idx + i), hnds.at(i));
				}
			}

			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				storage.reserve(old_size + count);
		
				for (std::size_t i = 0; i < back_count; ++i) {
					storage.emplace_back(std::forward<arg_Ts>(args)...);
				}
				
				std::move(storage.begin() + split, storage.begin() + idx + inplace_count, std::back_inserter(storage));

				std::for_each(storage.begin() + idx, count, [&](auto& val) {
					std::construct_at(&val, std::forward<arg_Ts>(args)...);
				});
				
				if constexpr (initialize_event_enabled) {
					for (std::size_t i = 0; i < count; ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i), component_at(idx + i));
					}
				}
			} else {
				if constexpr (initialize_event_enabled) {
					for (std::size_t i = 0; i < count; ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i));
					}
				}
			}
		}

		template<typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void _emplace_at(policy::strict, policy::immediate, std::size_t idx, std::span<handle_type> hnds, arg_Ts&&... args) {
			/*
			splits new elements into elements pushed to back and elements constructed
			inplace. All new elements beyond the bounds of the current range are
			emplaced to the back. old elements are moved beyond the range and new
			elements are constructed inplace.
			*/
			
			std::size_t old_size = size();										// size of pool before emplace
			std::size_t count = hnds.size();									// number of elements to emplace
			std::size_t back_count = std::max<int>(0, idx + count - old_size);	// the number of new elements pushed to back
			std::size_t inplace_count = count - back_count;						// the number of new elements constructed inplace
			std::size_t split = old_size - inplace_count;						// the index of the split between elements
			
			if constexpr (indexer_enabled) {
				auto& indexer = reg.template get_attribute<indexer_type>();
				
				for (std::size_t i = idx; i < old_size; ++i) {
					indexer.at(at(i)) = i + count;
				}
				for (std::size_t i = 0; i < count; ++i) {
					indexer.emplace(hnds.at(i), idx + i);
				}
			}
			
			if constexpr (manager_enabled) {
				auto& manager = reg.template get_attribute<manager_type>();
				manager.reserve(old_size + count);
				
				for (std::size_t i = 0; i < back_count; ++i) {
					manager.emplace_back(hnds.at(i + inplace_count));
				}

				std::move(manager.begin() + split, manager.begin() + old_size, std::back_inserter(manager));
				std::move_backward(manager.begin() + idx, manager.begin() + split, manager.begin() + old_size);

				for (std::size_t i = 0; i < back_count; ++i) {
					std::construct_at(&manager.at(idx + i), hnds.at(i));
				}
			}

			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();

				storage.reserve(old_size + count);

				for (std::size_t i = 0; i < back_count; ++i) {
					storage.emplace_back(std::forward<arg_Ts>(args)...);
				}
				
				std::move(storage.begin() + split, storage.begin() + old_size, std::back_inserter(storage));
				std::move_backward(storage.begin() + idx, storage.begin() + split, storage.begin() + old_size);

				for (std::size_t i = 0; i < inplace_count; ++i) {
					std::construct_at(&storage.at(idx + i), std::forward<arg_Ts>(args)...);
				}
				
				if constexpr (initialize_event_enabled) {
					for (std::size_t i = 0; i < count; ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i), storage.at(idx + i));
					}
				}
			} else {
				if constexpr (initialize_event_enabled) {
					for (std::size_t i = 0; i < count; ++i) {
						reg.template on<initialize_event>().invoke(hnds.at(i));
					}
				}
			}
		}

		constexpr void _erase_at(policy::optimal, policy::immediate, std::size_t idx, std::size_t count) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			auto& manager = reg.template get_attribute<manager_type>();
			
			if constexpr (terminate_event_enabled) {
				if constexpr (storage_enabled) {
					for (std::size_t i = idx, n = idx + count; i < n; ++i) {
						reg.template on<terminate_event>().invoke(at(i), component_at(i));
					} 
				} else {
					for (std::size_t i = idx, n = idx + count; i < n; ++i) {
						reg.template on<terminate_event>().invoke(at(i));
					} 
				}
			}
			
			auto manager_dst = manager.begin() + idx;
			auto manager_src = manager.end() - count;
			
			{ // indexer
				std::size_t i = idx;
				for (auto it = manager_src, end = manager.end(); it < end; ++it) {
					indexer.at(*it) = i++;
				}
	
				for (auto it = manager_dst, end = it + count; it < end; ++it) {
					indexer.erase(*it);
				}
			}

			{ // manager
				if (manager_src != manager_dst) {
					std::move(manager_src, manager.end(), manager_dst);
				}
				
				manager.erase(manager_src, manager.end());
			}
			
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				
				auto storage_dst = storage.begin() + idx;
				auto storage_src = storage.end() - count;

				if (storage_src != storage_dst) {
					std::move(storage_src, storage.end(), storage_dst);
				}
				
				storage.erase(storage_src, storage.end());
			}
		}

		constexpr void _erase_at(policy::strict, policy::immediate, std::size_t idx, std::size_t count) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			auto& manager = reg.template get_attribute<manager_type>();
			
			
			if constexpr (terminate_event_enabled) {
				if constexpr (storage_enabled) {
					for (std::size_t i = idx, n = idx + count; i < n; ++i) {
						reg.template on<terminate_event>().invoke(at(i), component_at(i));
					} 
				} else {
					for (std::size_t i = idx, n = idx + count; i < n; ++i) {
						reg.template on<terminate_event>().invoke(at(i));
					} 
				}
			}
			
			auto manager_dst = manager.begin() + idx;
			auto manager_src = manager_dst + count;
			auto manager_end = manager.end() - count;
			
			{ // indexer
				std::size_t i = idx;
				for (auto it = manager_src; it < manager.end(); ++it) {
					indexer.at(*it) = i -= count;
				}
	
				for (auto it = manager_dst, end = manager_src; it < end; ++it) {
					indexer.erase(*it);
				}
			}


			{ // manager
				if (manager_src != manager_dst) {
					std::move(manager_src, manager.end(), manager_dst);
				}
				
				manager.erase(manager_end, manager.end());
			}
			
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				
				auto storage_dst = manager.begin() + idx;
				auto storage_src = storage_dst + count;
				auto storage_end = storage.end() - count;

				if (storage_src != storage_dst) {
					std::move(storage_src, storage.end(), storage_dst);
				}

				storage.erase(storage_end, storage.end());
			}
		}

		constexpr void _clear(policy::immediate) {
			if constexpr (terminate_event_enabled) {
				auto invoker = reg.template on<terminate_event>();
				for (std::size_t pos = 0; pos < size(); ++pos) {
					
					if constexpr (storage_enabled) {
						invoker.invoke(at(pos), component_at(pos));
					} else {
						invoker.invoke(at(pos));
					}
				}
			}

			if constexpr (manager_enabled) {
				reg.template get_attribute<manager_type>().clear();
			}

			if constexpr (indexer_enabled) {
				reg.template get_attribute<indexer_type>().clear();
			}
			
			if constexpr (storage_enabled) {
				reg.template get_attribute<storage_type>().clear();
			}
		}
		
		// deferred policies

		constexpr void _emplace_back(ecs::policy::deferred, std::span<handle_type> hnds) requires (!storage_enabled || std::is_constructible_v<value_type>) {
			auto& manager = reg.template get_attribute<manager_type>();
			for (std::size_t i = 0; i < hnds.size(); ++i) {
				manager.emplace_back(hnds.at(i));
			}
		}

		constexpr void _emplace_at(policy::optimal, policy::deferred, std::size_t idx, std::span<handle_type> hnds) requires (!storage_enabled || std::is_constructible_v<value_type>) {
			std::size_t old_size = size();										// size of pool before emplace
			std::size_t count = hnds.size();									// number of elements to emplace
			std::size_t back_count = std::max<int>(0, idx + count - old_size);	// the number of elements pushed to back
			std::size_t inplace_count = count - back_count;						// the number of new elements constructed inplace
			std::size_t split = old_size - inplace_count;						// the index of the split between elements
			
			auto& manager = reg.template get_attribute<manager_type>();
			manager.reserve(old_size + count);

			for (std::size_t i = 0; i < back_count; ++i) {
				manager.emplace_back(hnds.at(i + inplace_count));
			}
			
			std::move(manager.begin() + split, manager.begin() + idx + inplace_count, std::back_inserter(manager));

			for (std::size_t i = 0; i < inplace_count; ++i) {
				std::construct_at(&manager.at(idx + i), hnds.at(i));
			}
		}
		
		constexpr void _emplace_at(policy::strict, policy::deferred, std::size_t idx, std::span<handle_type> hnds) requires (!storage_enabled || std::is_constructible_v<value_type>) {
			std::size_t old_size = size();										// size of pool before emplace
			std::size_t count = hnds.size();									// number of elements to emplace
			std::size_t back_count = std::max<int>(0, idx + count - old_size);	// the number of new elements pushed to back
			std::size_t inplace_count = count - back_count;						// the number of new elements constructed inplace
			std::size_t split = old_size - inplace_count;						// the index of the split between elements
			
			auto& manager = reg.template get_attribute<manager_type>();
			manager.reserve(old_size + count);
			
			for (std::size_t i = 0; i < back_count; ++i) {
				manager.emplace_back(hnds.at(i + inplace_count));
			}

			std::move(manager.begin() + split, manager.begin() + old_size, std::back_inserter(manager));
			std::move_backward(manager.begin() + idx, manager.begin() + split, manager.begin() + old_size);

			for (std::size_t i = 0; i < back_count; ++i) {
				std::construct_at(&manager.at(idx + i), hnds.at(i));
			}
		}

		//! constexpr void _erase(policy::optimal, policy::deferred, std::span<handle_type> hnds) { }

		//! constexpr void _erase(policy::strict, policy::deferred, std::span<handle_type> hnds) { }

		//! constexpr void _clear(policy::deferred) { }
		
	private:
		reg_T& reg;
	};
}