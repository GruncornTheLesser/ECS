#pragma once
#include "core/traits.h"
#include <cassert>
#include <span>

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

		using reference = std::add_lvalue_reference_t<value_type>;
		using const_reference = std::add_lvalue_reference_t<const value_type>;

		static constexpr bool initialize_event_enabled = !std::is_void_v<initialize_event>;
		static constexpr bool terminate_event_enabled = !std::is_void_v<terminate_event>;
		
		static constexpr bool manager_enabled = !std::is_void_v<manager_type>;
		static constexpr bool indexer_enabled = !std::is_void_v<indexer_type>;
		static constexpr bool storage_enabled = !std::is_void_v<storage_type>;

		// attributes cannot be shared
		static_assert(!manager_enabled || !indexer_enabled || !std::is_same_v<manager_type, indexer_type>);
		static_assert(!indexer_enabled || !storage_enabled || !std::is_same_v<indexer_type, storage_type>);
		static_assert(!storage_enabled || !manager_enabled || !std::is_same_v<storage_type, manager_type>);
		
		static_assert(indexer_enabled, "indexer disabled. Indexer required for all operations.");
		static_assert(!manager_enabled || storage_enabled, "storage enabled but manager disabled. storage requires manager attribute.");
		static_assert(!terminate_event_enabled || manager_enabled, "terminate event enabled but manager disabled. Events require manager attribute.");
		static_assert(!initialize_event_enabled || manager_enabled, "initialize event enabled but manager disabled. Events require manager attribute.");

	public:
		constexpr pool(reg_T& reg) noexcept : reg(reg) { }

		[[nodiscard]] constexpr std::size_t size() const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.size();
		}

		constexpr void reserve(std::size_t n) const {
			if constexpr (manager_enabled) {
				reg.template get_attribute<manager_type>().reserve(n);
			}

			if constexpr (storage_enabled) {
				reg.template get_attribute<storage_type>().reserve(n);
			}
		}

		/** returns the entity handle at the back of the pool */
		[[nodiscard]] constexpr const handle_type& back() const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.back();
		}
		
		/** returns the entity handle at the front of the pool */
		[[nodiscard]] constexpr const handle_type& front() const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.front();
		}

		/** returns the component at the back of the pool */
		[[nodiscard]] constexpr reference component_back() requires (storage_enabled) {
			auto& storage = reg.template get_attribute<storage_type>();
			return storage.back();
		}

		/** returns the component at the back of the pool */
		[[nodiscard]] constexpr reference component_back() const requires (storage_enabled) {
			const auto& storage = reg.template get_attribute<const storage_type>();
			return storage.at(size() - 1);
		}

		/** returns the component at the front of the pool */
		[[nodiscard]] constexpr reference component_front() requires (storage_enabled) {
			auto& storage = reg.template get_attribute<storage_type>();
			return storage.front();
		}

		/** returns the component at the front of the pool */
		[[nodiscard]] constexpr const_reference component_front() const requires (storage_enabled) {
			const auto& storage = reg.template get_attribute<const storage_type>();
			return storage.front();
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr const handle_type& at(std::size_t idx) const requires (manager_enabled) {
			const auto& manager = reg.template get_attribute<const manager_type>();
			return manager.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr reference component_at(std::size_t idx)  requires (storage_enabled) {
			auto& storage = reg.template get_attribute<storage_type>();
			return storage.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr const_reference component_at(std::size_t idx) const requires (storage_enabled) {
			const auto& storage = reg.template get_attribute<const storage_type>();
			return storage.at(idx);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr reference get_component(const handle_type& hnd) requires (indexer_enabled && storage_enabled) {
			return component_at(index_of(hnd));
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr const_reference get_component(const handle_type& hnd) const requires (indexer_enabled && storage_enabled) {
			return component_at(index_of(hnd));
		}
		
		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr bool get_component(const handle_type& hnd) requires (indexer_enabled && !storage_enabled) {
			return contains(hnd);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr const bool get_component(const handle_type& hnd) const requires (indexer_enabled && !storage_enabled) {
			return contains(hnd);
		}

		/** returns true if entity exists within the pool */
		[[nodiscard]] constexpr bool contains(const handle_type& hnd) const requires (indexer_enabled) {
			const auto& indexer = reg.template get_attribute<const indexer_type>();
			return indexer.contains(hnd);
		}

		/** returns the index of the component of a given entity */
		[[nodiscard]] constexpr std::size_t index_of(const handle_type& hnd) const requires (indexer_enabled && (manager_enabled || storage_enabled)) {
			const auto& indexer = reg.template get_attribute<const indexer_type>();
			if (auto it = indexer.find(hnd); it != indexer.end()) {
				return it->second;
			}

			return -1;
		}

		/** adds a component to the back of the pool */
		template<typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr decltype(auto) emplace_back(handle_type hnd, arg_Ts&&... args) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			
			if constexpr (manager_enabled) {
				auto& manager = reg.template get_attribute<manager_type>();
				
				manager.emplace_back(hnd);
				
				indexer.emplace(hnd, indexer.size());

				if constexpr (storage_enabled) {
					auto& storage = reg.template get_attribute<storage_type>();
					
					auto& val = storage.emplace_back(std::forward<arg_Ts>(args)...);

					if constexpr (initialize_event_enabled) {
						reg.template on<initialize_event>().invoke(hnd, val);
					}

					return val;
				} else {
					if constexpr (initialize_event_enabled) {
						reg.template on<initialize_event>().invoke(hnd);
					}
				}
			} else {
				indexer.emplace(hnd);
			}
		}

		/** adds a component to the back of the pool */
		template<typename ... arg_Ts> requires (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr void emplace_back(std::span<handle_type> hnds, arg_Ts&&... args) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			
			if constexpr (manager_enabled) { // manager
				auto& manager = reg.template get_attribute<manager_type>();

				manager.reserve(manager.size() + hnds.size());
				
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					manager.emplace_back(hnds[i]);
				}
				
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					indexer.emplace(hnds[i], indexer.size());
				}

				if constexpr (storage_enabled) {
					auto& storage = reg.template get_attribute<storage_type>();
					storage.reserve(storage.size() + hnds.size());
					
					for (std::size_t i = 0; i < hnds.size(); ++i) {
						storage.emplace_back(std::forward<arg_Ts>(args)...);
					}

					if constexpr (initialize_event_enabled) {
						auto it = storage.end() - hnds.size();
						for (std::size_t i = 0; i < hnds.size(); ++i) {
							reg.template on<initialize_event>().invoke(hnds[i], *it++);
						}
					}	
				} else {
					if constexpr (initialize_event_enabled) {
						for (std::size_t i = 0; i < hnds.size(); ++i) {
							reg.template on<initialize_event>().invoke(hnds[i]);
						}
					}
				}
			} else {
				for (std::size_t i = 0; i < hnds.size(); ++i) {
					indexer.emplace(hnds[i]);
				}
			}
		}
		
		/** adds a component to the pool at the index */
		template<typename seq_T=policy::optimal, typename ... arg_Ts> requires (manager_enabled && (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>))
		constexpr decltype(auto) emplace_at(std::size_t idx, handle_type hnd, arg_Ts&&... args) {
			auto& manager = reg.template get_attribute<manager_type>();
			auto& indexer = reg.template get_attribute<indexer_type>();
			seq_T policy;

			manager.reserve(manager.size() + 1);

			auto pos = manager.begin() + idx;
			auto update = policy.emplace(manager, pos, hnd);

			indexer.emplace(hnd, std::distance(manager.begin(), pos));
			
			for (auto it = update.begin(), end = manager.end(); it < end; ++it) {
				indexer.at(*it) = std::distance(manager.begin(), it);
			}

			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				storage.reserve(storage.size() + 1);

				policy.emplace(storage, storage.begin() + idx, std::forward<arg_Ts>(args)...);
			
				if constexpr (initialize_event_enabled) {
					reg.template on<initialize_event>().invoke(hnd, storage.at(idx));
				}
				
				return storage.at(idx);
			} else {
				if constexpr (initialize_event_enabled) {
					reg.template on<initialize_event>().invoke(hnd);
				}
			}
		}

		/** adds a component to the pool at the index */
		template<typename seq_T=policy::optimal, typename ... arg_Ts> requires (manager_enabled && (!storage_enabled || std::is_constructible_v<value_type, arg_Ts...>))
		constexpr void emplace_at(std::size_t idx, std::span<handle_type> hnds, arg_Ts&&... args) {
			auto& manager = reg.template get_attribute<manager_type>();
			auto& indexer = reg.template get_attribute<indexer_type>();
			seq_T policy;

			manager.reserve(manager.size() + hnds.size());

			auto pos = manager.begin() + idx;
			auto update = policy.insert_range(manager, hnds);

			for (auto it = pos, end = it + hnds.size(); it < end; ++it) {
				indexer.emplace(*it, std::distance(manager.begin(), it));
			}
			
			for (auto it = update, end = manager.end(); it < end; ++it) {
				indexer.at(*it) = std::distance(manager.begin(), it);
			}

			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				storage.reserve(storage.size() + hnds.size());
				policy.emplace_n(storage, storage.begin() + idx, hnds.size(), std::forward<arg_Ts>(args)...);
			
				if constexpr (initialize_event_enabled) {
					for (auto hnd_it = hnds.begin() + idx, val_it = storage.begin() + idx, end = hnds.end(); hnd_it != end; ++hnd_it, ++val_it) {
						reg.template on<initialize_event>().invoke(*hnd_it, *val_it);
					}
				}
			} else {
				if constexpr (initialize_event_enabled) {
					for (auto hnd_it = hnds.begin() + idx, end = hnds.end(); hnd_it != end; ++hnd_it) {
						reg.template on<initialize_event>().invoke(*hnd_it);
					}
				}
			}
		}

		/** erases a component from the pool */
		template<typename seq_T=policy::optimal>
		constexpr void erase(handle_type hnd) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			seq_T policy;
			
			if constexpr (manager_enabled) {
				auto& manager = reg.template get_attribute<manager_type>();

				std::size_t idx = index_of(hnd);

				if constexpr (terminate_event_enabled) {
					if constexpr (storage_enabled) {
						reg.template on<terminate_event>().invoke(at(idx), component_at(idx)); 
					} else {
						reg.template on<terminate_event>().invoke(at(idx)); 
					}
				}

				indexer.erase(hnd);
				
				auto update = policy.erase(manager, manager.begin() + idx);

				for (auto it = update.begin(), end = update.end(); it < end; ++it) {
					indexer.at(*it) = std::distance(manager.begin(), it);
				}
				
				if constexpr (storage_enabled) {
					auto& storage = reg.template get_attribute<storage_type>();
					policy.erase(storage, storage.begin() + idx);
				}
			} else {
				indexer.erase(hnd);
			}

			
		}

		/** erases a component from the pool */
		template<typename seq_T=policy::optimal>
		constexpr void erase(std::span<handle_type> hnds) {
			for (auto& hnd : hnds) erase<seq_T>(hnd); // TODO: improve performance, execute rolling move from sorted handles
		}

		/** erases a component at an index */
		template<typename seq_T=policy::optimal>
		constexpr void erase_at(std::size_t idx) requires (manager_enabled) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			auto& manager = reg.template get_attribute<manager_type>();
			seq_T policy;

			if constexpr (terminate_event_enabled) {
				if constexpr (storage_enabled) {
					reg.template on<terminate_event>().invoke(at(idx), component_at(idx)); 
				} else {
					reg.template on<terminate_event>().invoke(at(idx)); 
				}
			}
			
			auto pos = manager.begin() + idx;

			indexer.erase(*pos);
						
			auto update = policy.erase(manager, pos);

			for (auto it = update.begin(), end = update.end(); it < end; ++it) {
				indexer.at(*it) = std::distance(manager.begin(), it);
			}
			
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				policy.erase(storage, storage.begin() + idx);
			}
		}

		/** erases a component at an index */
		template<typename seq_T=policy::optimal>
		constexpr void erase_at(std::size_t idx, std::size_t count) requires (manager_enabled) {
			auto& indexer = reg.template get_attribute<indexer_type>();
			auto& manager = reg.template get_attribute<manager_type>();
			seq_T policy;

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
			
			auto pos = manager.begin() + idx;

			for (auto it = pos, end = pos + count; it < end; ++it) {
				indexer.erase(*it);
			}
			
			auto update = policy.erase_n(manager, pos, count);

			for (auto it = update.begin(), end = update.end(); it < end; ++it) {
				indexer.at(*it) = std::distance(manager.begin(), it);
			}
			
			if constexpr (storage_enabled) {
				auto& storage = reg.template get_attribute<storage_type>();
				policy.erase_n(storage, storage.begin() + idx, count);
			}
		}

		/** removes all components from the pool */
		template<typename seq_T=policy::optimal>
		constexpr void clear() {
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

			if constexpr (storage_enabled) {
				reg.template get_attribute<storage_type>().clear();
			}

			if constexpr (manager_enabled) {
				reg.template get_attribute<manager_type>().clear();
			}

			{ // indexer type
				reg.template get_attribute<indexer_type>().clear();
			}
		}
		
	private:
		reg_T& reg;
	};
	
}