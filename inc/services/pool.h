#pragma once
#include "core/traits.h"
#include <span>
#include <tuple>

namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
	private:
		using component_type = std::remove_const_t<T>;
		using registry_type = reg_T;

		using entity_type = traits::component::get_entity_t<component_type>;
		using handle_type = traits::component::get_handle_t<component_type>;
		using value_type = util::copy_const_t<traits::component::get_value_t<component_type>, T>;

		using initialize_type = traits::component::get_initialize_event_t<component_type>;
		using terminate_type = traits::component::get_terminate_event_t<component_type>;
		
		using manager_type = util::copy_const_t<traits::component::get_manager_t<component_type>, T>;
		using indexer_type = util::copy_const_t<traits::component::get_indexer_t<component_type>, T>;
		using storage_type = util::copy_const_t<traits::component::get_storage_t<component_type>, T>;
		
		static constexpr bool entity_enabled = !std::is_void_v<entity_type>;
		static constexpr bool manager_enabled = !std::is_void_v<manager_type>;
		static constexpr bool indexer_enabled = !std::is_void_v<indexer_type>;
		static constexpr bool storage_enabled = !std::is_void_v<storage_type>;
		
		static constexpr bool event_initialize_enabled = !std::is_void_v<traits::component::get_initialize_event_t<T>>;
		static constexpr bool event_terminate_enabled = !std::is_void_v<traits::component::get_terminate_event_t<T>>;
		
		static constexpr bool shared_manager = manager_enabled && (std::is_same_v<manager_type, indexer_type> || std::is_same_v<manager_type, storage_type>);
		static constexpr bool shared_indexer = indexer_enabled && (std::is_same_v<indexer_type, manager_type> || std::is_same_v<indexer_type, storage_type>);
		static constexpr bool shared_storage = storage_enabled && (std::is_same_v<storage_type, manager_type> || std::is_same_v<storage_type, indexer_type>);
		
		static constexpr bool shared_manager_indexer_storage = manager_enabled && std::is_same_v<manager_type, indexer_type> && std::is_same_v<manager_type, storage_type>; 
		static constexpr bool shared_manager_indexer = manager_enabled && std::is_same_v<manager_type, indexer_type>; // set<handle_type>
		static constexpr bool shared_indexer_storage = indexer_enabled && std::is_same_v<indexer_type, storage_type>; // map<handle_type, value_type>
		static constexpr bool shared_manager_storage = storage_enabled && std::is_same_v<manager_type, storage_type>; // std::vector<std::pair<handle_type, value_type>>
	public:
		inline constexpr pool(reg_T& reg) noexcept : reg(reg) { }

		/** the number of active + inactive components */
		[[nodiscard]] constexpr std::size_t size() const {
			if constexpr (manager_enabled) {
				return reg.template get_attribute<manager_type>().size();
			} else if constexpr (storage_enabled) {
				return reg.template get_attribute<storage_type>().size();
			} else {
				return reg.template get_attribute<indexer_type>().size();
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline handle_type at(std::size_t ind) requires (manager_enabled) { 
			if constexpr (shared_manager) {
				return reg.template get_attribute<manager_type>().at(ind).first;
			} else {
				return reg.template get_attribute<manager_type>().at(ind);
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline const handle_type at(std::size_t ind) const requires (manager_enabled) { 
			if constexpr (shared_manager) {
				return reg.template get_attribute<manager_type>().at(ind).first;
			} else {
				return reg.template get_attribute<manager_type>().at(ind);
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline value_type& component_at(std::size_t ind) requires (storage_enabled) { 
			if constexpr (shared_storage) {
				return reg.template get_attribute<storage_type>().at(ind).second;
			} else {
				return reg.template get_attribute<storage_type>().at(ind);
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline const value_type& component_at(std::size_t ind) const requires (storage_enabled) { 
			if constexpr (shared_storage) {
				return reg.template get_attribute<storage_type>().at(ind).second;
			} else {
				return reg.template get_attribute<storage_type>().at(ind);
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline value_type& get_component(handle_type hnd) requires (storage_enabled) { 
			if constexpr (shared_indexer_storage) {
				return reg.template get_attribute<storage_type>().find(hnd)->second;
			} else {
				return component_at(index_of(hnd));
			}			
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline const value_type& get_component(handle_type hnd) const requires (storage_enabled) { 
			if constexpr (shared_indexer_storage) {
				return reg.template get_attribute<storage_type>().find(hnd)->second;
			} else {
				return component_at(index_of(hnd));
			}
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline bool get_component(handle_type hnd) requires (!storage_enabled) {
			return contains(hnd);
		}

		/** returns the entity handle at the given index */
		[[nodiscard]] constexpr inline bool get_component(handle_type hnd) const requires (!storage_enabled) {
			return contains(hnd);
		}

		/** returns true if entity exists within the pool */
		[[nodiscard]] constexpr inline bool contains(handle_type ent) const requires (indexer_enabled || manager_enabled) {
			if constexpr (indexer_enabled) {
				return reg.template get_attribute<const indexer_type>().contains(ent);
			} else if constexpr (shared_manager) {
				return std::ranges::contains(reg.template get_attribute<const manager_type>(), ent, [](const auto& p) { return p.first; });
			} else {
				return std::ranges::contains(reg.template get_attribute<const manager_type>(), ent);
			}
		}

		/** returns the index of the component of a given entity */
		[[nodiscard]] constexpr std::size_t index_of(handle_type hnd) const requires (manager_enabled || (indexer_enabled && storage_enabled)) {
			if constexpr (indexer_enabled) {
				const auto& indexer = reg.template get_attribute<const indexer_type>();
				
				if (auto it = indexer.find(hnd); it != indexer.end()) { 
					if constexpr (shared_indexer || (!manager_enabled && !storage_enabled)) {
						return std::distance(indexer.begin(), it);
					} else {
						return it->second;
					} 	
				}
			} else if constexpr (manager_enabled) {
				const auto& manager = reg.template get_attribute<const manager_type>();
				if constexpr (shared_manager_storage) {
					if (auto it = std::ranges::find(manager, hnd, [](auto& p) { return p.first; }); it != manager.end()) {
						return std::distance(manager.begin(), it);
					}
				} else {
					if (auto it = std::ranges::find(manager, hnd); it != manager.end()) {
						return std::distance(manager.begin(), it);
					}
				}
			} else {
				static_assert(false);
			}

			return -1;
		}

		// modifiers
		/** adds a component to an entity */
		template<typename ... arg_Ts>
		constexpr value_type& emplace(handle_type hnd, arg_Ts&&... args) requires (entity_enabled && storage_enabled) {
			value_type* ptr;

			if constexpr (indexer_enabled) {
				auto& indexer = reg.template get_attribute<indexer_type>();

				if constexpr (shared_indexer_storage) {
					ptr = &indexer.try_emplace(hnd, std::forward<arg_Ts>(args)...).first->second;
				} else if constexpr (shared_manager_indexer) {
					indexer.emplace(hnd);
				} else {
					indexer.emplace(hnd, size());
				}
			}
			
			if constexpr (manager_enabled && !shared_manager_indexer) {
				auto& manager = reg.template get_attribute<manager_type>();
				
				if constexpr (shared_manager_storage) {
					ptr = &manager.emplace_back(std::piecewise_construct, std::forward_as_tuple(hnd), std::forward_as_tuple(args...)).second;
				} else {
					manager.emplace_back(std::forward<arg_Ts>(args)...);
				}
			}

			if constexpr (!shared_manager_storage && !shared_indexer_storage) {
				ptr = &reg.template get_attribute<storage_type>().emplace_back(std::forward<arg_Ts>(args)...);
			}

			if constexpr (event_initialize_enabled) {
				reg.template on<initialize_type>().invoke(hnd, *ptr);
			}

			return *ptr;
		}

		template<typename ... arg_Ts>
		constexpr void emplace(handle_type hnd, arg_Ts&&... args) requires (entity_enabled && !storage_enabled) {
			if constexpr (indexer_enabled) {
				auto& indexer = reg.template get_attribute<indexer_type>();

				if constexpr (shared_manager_indexer || !manager_enabled) { // AA-, A--
					indexer.emplace(hnd);
				} else {
					indexer.emplace(hnd, size());
				}
			}
			
			if constexpr (manager_enabled && !shared_manager_indexer) {
				reg.template get_attribute<manager_type>().emplace_back(std::forward<arg_Ts>(args)...);
			}

			if constexpr (event_initialize_enabled) {
				reg.template on<initialize_type>().invoke(hnd);
			}
		}

		template<typename ... arg_Ts>
		constexpr std::pair<handle_type, value_type&> emplace(arg_Ts&&... args) requires (!entity_enabled && storage_enabled);

		template<typename ... arg_Ts>
		constexpr void emplace(handle_type hnd, arg_Ts&&... args) requires (!entity_enabled && !storage_enabled) {
			if constexpr (indexer_enabled) {
				auto& indexer = reg.template get_attribute<indexer_type>();

				if constexpr (shared_manager_indexer || !manager_enabled) {
					indexer.emplace(hnd);
				} else {
					indexer.emplace(hnd, size());
				}
			}
			
			if constexpr (manager_enabled && !shared_manager_indexer) {
				reg.template get_attribute<manager_type>().emplace_back(std::forward<arg_Ts>(args)...);
			}

			if constexpr (event_initialize_enabled) {
				reg.template on<initialize_type>().invoke(hnd);
			}
		}

		/** adds a component to each entity */
		//template<typename ... arg_Ts>
		//constexpr void emplace(std::span<handle_type> ents, arg_Ts&&... args) { }
		
		//template<typename ... arg_Ts>
		//constexpr decltype(auto) emplace_at(std::size_t ind, handle_type ent, arg_Ts&&... args) { }
		
		//template<typename ... arg_Ts> requires (std::is_void_v<storage_type> || std::is_constructible_v<value_type, arg_Ts...>)
		//constexpr void emplace_at(std::size_t ind, std::span<handle_type> ents, arg_Ts&&... args) { }

		constexpr void erase(handle_type ent) { 
			// event_terminate_enabled
		}

		//constexpr void erase_at(std::size_t hnd) { }

		//constexpr void erase(std::span<handle_type> ents) { }

		//constexpr void erase_at(std::span<std::size_t> inds) { }

		constexpr void clear() {
			if constexpr (event_terminate_enabled) { // invoke terminate event
				auto invoker = reg.template on<event::terminate<T>>();
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
			if constexpr (indexer_enabled && !shared_manager_indexer) {
				reg.template get_attribute<indexer_type>().clear();
			}
			if constexpr (indexer_enabled && !shared_manager_storage && !shared_indexer_storage) {
				reg.template get_attribute<storage_type>().clear();
			}
		}


	private:
		reg_T& reg;
	};
}
