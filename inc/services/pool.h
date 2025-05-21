#pragma once
#include "core/traits.h"

namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
		using component_type = std::remove_const_t<T>;
		using registry_type = reg_T;

		using value_type = traits::component::get_value_t<component_type>;
		using entity_type = traits::component::get_entity_t<component_type>;
		using manager_type = traits::component::get_manager_t<component_type>;
		using indexer_type = traits::component::get_indexer_t<component_type>;
		using storage_type = traits::component::get_storage_t<component_type>;

		using handle_type = traits::entity::get_handle_t<entity_type>;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
		using integral_type = typename handle_type::integral_type;
		
		static constexpr bool entity_enabled =  !std::is_same_v<entity_type,  void>;
		static constexpr bool manager_enabled = !std::is_same_v<manager_type, void>;
		static constexpr bool indexer_enabled = !std::is_same_v<indexer_type, void>;
		static constexpr bool storage_enabled = !std::is_same_v<storage_type, void>;
		
		static constexpr bool event_term_enabled = traits::is_accessible_v<registry_type, const event::term<T>>;
		static constexpr bool event_init_enabled = traits::is_accessible_v<registry_type, const event::init<T>>;
		
		static_assert(manager_enabled);
		
	public:
		inline constexpr pool(reg_T* reg) noexcept : reg(reg) { }

		inline constexpr auto begin() noexcept { return reg->template view<entity_type, T>().begin(); }
		inline constexpr auto end() noexcept { return reg->template view<entity_type, T>().end(); }
		inline constexpr auto rbegin() noexcept { return reg->template view<entity_type, T>().rbegin(); }
		inline constexpr auto rend() noexcept { return reg->template view<entity_type, T>().rend(); }
		
		// capacity
		[[nodiscard]] inline constexpr bool empty() const noexcept { return size() == 0; }
		inline constexpr std::size_t size() const { 
			return reg->template get_resource<manager_type>().size();
		}
		
		[[nodiscard]] constexpr integral_type index_of(handle_type ent) {
			if constexpr (indexer_enabled) {
				auto& indexer = reg->template get_resource<indexer_type>();
				if (auto it = indexer.find(value_view{ ent }); it != indexer.end()) {
					if (version_view{ it->second } == version_view{ ent }) {
						return value_view{ it->second };
					}
				}
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				if (auto it = std::ranges::find(manager, ent); it != manager.end()) {
					return it - manager.begin();
				}
			} 
			return -1;
		}

		[[nodiscard]] bool contains(handle_type ent) {
			if constexpr (indexer_enabled) {
				const auto& indexer = reg->template get_resource<indexer_type>();
				auto it = indexer.find(value_view{ ent });
				return (it != indexer.end()) && (version_view{ it->second } == version_view{ ent });
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				return std::ranges::find(manager, ent) != manager.end();
			}
		}

		// modifiers
		template<typename ... arg_Ts> requires (std::is_constructible_v<traits::component::get_value_t<T>, arg_Ts...>)
		decltype(auto) emplace(handle_type ent, arg_Ts&&... args) {
			if constexpr (indexer_enabled) {
				integral_type back = size();

				auto& indexer = reg->template get_resource<indexer_type>();
				indexer[value_view{ ent }] = handle_type{ back, ent };
			}
			
			if constexpr (manager_enabled) {
				auto& manager = reg->template get_resource<manager_type>();
				manager.emplace_back(ent);
			}

			if constexpr (storage_enabled) {
				auto& storage = reg->template get_resource<storage_type>();
				auto& component = storage.emplace_back(std::forward<arg_Ts>(args)...);
				
				if constexpr (event_init_enabled) {
					reg->template on<event::init<T>>().invoke(ent, component);
				}

				return component;
			} else {
				if constexpr (event_init_enabled) {
					reg->template on<event::init<T>>().invoke(ent);
				}
			}
		}

		constexpr bool erase(handle_type ent) {
			integral_type back = size() - 1;
			integral_type pos = index_of(ent);
			
			if (pos == -1) return false;
			
			if (pos != back) {
				if constexpr (indexer_enabled) {
					auto& indexer = reg->template get_resource<indexer_type>();
					handle_type back_ent = reg->template get_resource<manager_type>()[back];
					indexer.at(value_view{ back_ent }) = { static_cast<integral_type>(pos), version_view{ back_ent } };
				}

				if constexpr (manager_enabled) {
					auto& manager = reg->template get_resource<manager_type>();
					if constexpr (std::is_trivially_destructible_v<T>) {
						manager[pos] = std::move(manager[back]);
					} else {
						std::swap(manager[pos], manager[back]);
					}
				}
				
				if constexpr (storage_enabled) {
					auto& storage = reg->template get_resource<storage_type>();
					if constexpr (std::is_trivially_destructible_v<T>) {
						storage[pos] = std::move(storage[back]);
					} else {
						std::swap(storage[pos], storage[back]);
					}
				}
			}
			if constexpr (indexer_enabled) {
				auto& indexer = reg->template get_resource<indexer_type>();
				indexer.erase(value_view{ ent });
			}
			if constexpr (manager_enabled) {
				reg->template get_resource<manager_type>().pop_back();
			}
			if constexpr (storage_enabled) {
				reg->template get_resource<storage_type>().pop_back();
			}
			
			if constexpr (event_term_enabled) {
				if constexpr (storage_enabled) {
					reg->template on<event::term<T>>().invoke(ent, reg->template get_resource<storage_type>()[pos]);
				} else {
					reg->template on<event::term<T>>().invoke(ent);
				}
			}

			return true;
		}

		constexpr void clear() {			
			if constexpr (event_term_enabled) {
				if constexpr (storage_enabled) {
					auto& manager = reg->template get_resource<manager_type>();
					auto& storage = reg->template get_resource<storage_type>();
					
					auto invoker = reg->template on<event::term<T>>();
					for (std::size_t pos = 0; pos < size(); ++pos) {
						invoker.invoke(manager[pos], storage[pos]);
					}
				} else {
					auto& manager = reg->template get_resource<manager_type>();
					
					auto invoker = reg->template on<event::term<T>>();
					for (std::size_t pos = 0; pos < size(); ++pos) {
						invoker.invoke(manager[pos]);
					}
				}
			}

			if constexpr (manager_enabled) {
				reg->template get_resource<manager_type>().clear();
			}
			if constexpr (indexer_enabled) {
				reg->template get_resource<indexer_type>().clear();
			}
			if constexpr (storage_enabled) {
				reg->template get_resource<storage_type>().clear();
			}
		}

	private:
		reg_T* reg;
	};
}
