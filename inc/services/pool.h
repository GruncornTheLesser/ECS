#pragma once
#include "core/traits.h"
#include <stdexcept>

namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
	public:
		using registry_type = reg_T;
		using value_type = traits::component::get_value_t<T>;
		using entity_type = traits::component::get_entity_t<T>;
		using manager_type = traits::component::get_manager_t<T>;
		using indexer_type = traits::component::get_indexer_t<T>;
		using storage_type = traits::component::get_storage_t<T>;

		using manager_component_set = util::filter_t<typename reg_T::component_set, util::pred_<traits::is_manager_match, manager_type>::template type>;
		using indexer_component_set = util::filter_t<typename reg_T::component_set, util::pred_<traits::is_manager_match, manager_type>::template type>;
		using storage_component_set = util::filter_t<typename reg_T::component_set, util::pred_<traits::is_manager_match, manager_type>::template type>;

		using handle_type = traits::entity::get_handle_t<entity_type>;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
		using integral_type = typename handle_type::integral_type;
	
	private:
		static constexpr bool manager_enabled = !std::is_same_v<manager_type, void>;
		static constexpr bool indexer_enabled = !std::is_same_v<indexer_type, void>;
		static constexpr bool storage_enabled = !std::is_same_v<storage_type, void>;

		static constexpr bool indexer_contiguous = std::ranges::contiguous_range<traits::resource::get_value_t<indexer_type>>;

		static constexpr bool enable_term_event = traits::is_accessible_v<event::term<T>, registry_type>;
		static constexpr bool enable_init_event = traits::is_accessible_v<event::init<T>, registry_type>;
		
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
			if      constexpr (storage_enabled) { return reg->template get_resource<storage_type>().size(); }
			else if constexpr (manager_enabled) { return reg->template get_resource<manager_type>().size(); }
			else if constexpr (indexer_enabled) { return reg->template get_resource<indexer_type>().count(); }
		}

		constexpr void reserve(std::size_t n) {
			if constexpr (manager_enabled) { 
				reg->template get_resource<manager_type>().reserve(n); 
			}
			
			if constexpr (indexer_enabled && indexer_contiguous) { // TODO: if indexer sparse or contiguous
				reg->template get_resource<indexer_type>().reserve(n);
			}
			if constexpr (storage_enabled) { 
				reg->template get_resource<storage_type>().reserve(n); 
			}
		}
		constexpr void shrink_to_fit() {
			if constexpr (manager_enabled) {
				reg->template get_resource<manager_type>().shrink_to_fit();
			}

			if constexpr (indexer_enabled && indexer_contiguous) {
				reg->template get_resource<indexer_type>().shrink_to_fit();
			}

			if constexpr (storage_enabled) {
				reg->template get_resource<storage_type>().shrink_to_fit();
			}
		}
		
		// element/page/data access
		inline constexpr auto operator[](std::size_t pos) {
			return reg->template view<entity_type, T>()[pos];
		}	
		
		inline constexpr auto operator[](handle_type ent) {
			if constexpr (indexer_enabled) {
				auto key = value_view{ ent };
				auto pos = reg->template get_resource<indexer_type>().at(key);
				return operator[](value_view{ pos });
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				return operator[](std::ranges::find(manager, ent) - manager.begin());
			}
		}
		
		constexpr auto at(std::size_t pos) {
			if (pos >= size()) throw std::out_of_range("");
			return operator[](pos);
		}

		constexpr value_type& get_component(handle_type ent) requires (storage_enabled) {
			return reg->template get_resource<storage_type>().at(index_of(ent));
		}

		constexpr auto front() {
			if (empty()) throw std::out_of_range("");
			return operator[](0);
		}
		
		constexpr auto back() {
			if (empty()) throw std::out_of_range("");
			return operator[](size() - 1);
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
				auto& indexer = reg->template get_resource<indexer_type>();
				auto it = indexer.find(value_view{ ent });
				return (it != indexer.end()) && (version_view{ it->second } == version_view{ ent });
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				return std::ranges::find(manager, ent) != manager.end();
			}
		}

		// modifiers
		template<typename ... arg_Ts> requires (std::is_constructible_v<T, arg_Ts...>)
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
				
				if constexpr (enable_init_event) {
					reg->template on<event::init<T>>().invoke(ent, component);
				}

				return component;
			} else {
				if constexpr (enable_init_event) {
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
			
			if constexpr (enable_term_event) {
				if constexpr (storage_enabled) {
					reg->template on<event::term<T>>().invoke(ent, reg->template get_resource<storage_type>()[pos]);
				} else {
					reg->template on<event::term<T>>().invoke(ent);
				}
			}

			return true;
		}

		constexpr void clear() {			
			if constexpr (enable_term_event) {
				for (std::size_t pos = 0; pos < size(); ++pos) {
					if constexpr (storage_enabled) {
						reg->template on<event::term<T>>().invoke(
							reg->template get_resource<manager_type>()[pos], 
							reg->template get_resource<storage_type>()[pos]
						);
					} else {
						reg->template on<event::term<T>>().invoke(
							reg->template get_resource<manager_type>()[pos]
						);
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
