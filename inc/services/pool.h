#pragma once
#include "core/traits.h"
#include <stdexcept>
#include <span>

// things you can do
// - disable storage by setting storage_type to void
// - share lookup and management by setting manager_type and indexer_type to the same resource
// ! size() attribute cannot be found if storage_type is void and indexer_type is the same as manager_type
// ! reserved() == size() it would definitely break the insertion and erase synchronization and storage
// ^ solution: create null_storage type? which is an empty interface for a storage container simply holding 'std::size_t size() const'

namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
	private:
		using component_type = std::remove_const_t<T>;
		using registry_type = reg_T;

		using entity_type = traits::component::get_entity_t<component_type>;
		using value_type = util::copy_const_t<traits::component::get_value_t<component_type>, T>;
		using manager_type = util::copy_const_t<traits::component::get_manager_t<component_type>, T>;
		using indexer_type = util::copy_const_t<traits::component::get_indexer_t<component_type>, T>;
		using storage_type = util::copy_const_t<traits::component::get_storage_t<component_type>, T>;	

		using handle_type = traits::entity::get_handle_t<entity_type>;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
				
		static constexpr bool event_initialize_enabled = !std::is_void_v<traits::component::get_initialize_event_t<T>>;
		static constexpr bool event_terminate_enabled = !std::is_void_v<traits::component::get_terminate_event_t<T>>;

	public:
		inline constexpr pool(reg_T* reg) noexcept : reg(reg) { }

		inline constexpr auto begin() noexcept { return reg->template view<entity_type, T>().begin(); }
		inline constexpr auto end() noexcept { return reg->template view<entity_type, T>().end(); }
		inline constexpr auto rbegin() noexcept { return reg->template view<entity_type, T>().rbegin(); }
		inline constexpr auto rend() noexcept { return reg->template view<entity_type, T>().rend(); }
		
		/** @brief the count of active components in the pool */
		[[nodiscard]] inline constexpr std::size_t size() const {
			return reg->template get_resource<const storage_type>().size();
		}
		/** @brief the number of active + inactive components */
		[[nodiscard]] constexpr std::size_t reserved() const { 
			return reg->template get_resource<const manager_type>().size();
		}
		/** @brief the number of allocated components */
		[[nodiscard]] constexpr std::size_t capacity() const { 
			return reg->template get_resource<const manager_type>().capacity();
		}
		/** @brief return true if size() equals zero */
		[[nodiscard]] inline constexpr bool empty() const noexcept {
			return size() == 0;
		}
		/** @brief returns the index of the component of a given entity */
		[[nodiscard]] constexpr std::size_t index_of(handle_type ent) const {
			if constexpr (requires { reg->template get_resource<const indexer_type>().find(ent); }) {
				const auto& indexer = reg->template get_resource<const indexer_type>();
				if (auto it = indexer.find(ent); it != indexer.end() && version_view{ it->second } == version_view{ ent }) {
					return value_view{ it->second };
				} 
				return -1;
			}
			
			else if (requires { std::ranges::find(reg->template get_resource<const manager_type>(), ent); }) {
				const auto& manager = reg->template get_resource<const manager_type>();
				if (auto it = std::find(manager.begin(), manager.begin() + size(), ent); it != manager.end()) {
					return it - manager.begin();
				}
				return -1;
			} 
			
			else static_assert(false);
		}

		/** @brief returns true if entity exists within the pool */
		[[nodiscard]] constexpr bool contains(handle_type ent) const {

			if constexpr (requires { reg->template get_resource<const indexer_type>(); }) {
				const auto& indexer = reg->template get_resource<const indexer_type>();

				if constexpr (requires { indexer.contains(ent); }) {
					return indexer.contains(ent);
				}
				else if constexpr (requires { indexer.find(ent); }) {
					return (indexer.find(std::size_t{ ent }) != indexer.end());
				}
			}

			if constexpr (requires { reg->template get_resource<const manager_type>(); }) {
				const auto& manager = reg->template get_resource<const manager_type>();

				if constexpr (requires { manager.contains(ent); }) {
					return manager.contains(ent);
				}
				else if constexpr (requires { manager.find(value_view{ ent }); }) {
					return manager.find(std::size_t{ ent }) != manager.end();
				}
				else if constexpr (requires { std::find(manager.begin(), manager.begin() + size(), ent); }) {
					auto begin = manager.begin();
					auto end = manager.begin() + size();
					return std::find(begin, end, ent) != end;
				}
				return -1;
			}
		}

		/** @brief returns the entity at the given index */
		[[nodiscard]] inline handle_type at(std::size_t ind) {
			return reg->template get_resource<const manager_type>().at(ind);
		}
		
		/** @brief returns the component at the given index */
		[[nodiscard]] inline value_type& get_at(std::size_t ind) {
			return reg->template get_resource<storage_type>().at(ind);
		}

		/** @brief returns the component of the given entity */
		[[nodiscard]] inline value_type& get(handle_type hnd) {
			return get_at(index_of(hnd));
		}
			
		// modifiers
		/** @brief queues a component to be added to the container */
		template<typename ... arg_Ts> requires (std::is_void_v<storage_type> || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr decltype(auto) emplace(handle_type ent, arg_Ts&&... args) { }
		
		// template<typename ... arg_Ts> requires (std::is_void_v<storage_type> || std::is_constructible_v<value_type, arg_Ts...>)
		// constexpr decltype(auto) emplace(std::span<handle_type> ents, arg_Ts&&... args) { }
		
		template<typename ... arg_Ts> requires (std::is_void_v<storage_type> || std::is_constructible_v<value_type, arg_Ts...>)
		constexpr decltype(auto) emplace_at(std::size_t ind, handle_type ent, arg_Ts&&... args) { }

		// template<typename seq_T=policy::swap_pop, typename ... arg_Ts> requires (std::is_void_v<storage_type> || std::is_constructible_v<value_type, arg_Ts...>)
		// constexpr decltype(auto) emplace_at(std::size_t ind, std::span<handle_type> ents, arg_Ts&&... args) { }

		
		constexpr void erase(handle_type ent) { }

		// template<typename exec_T=ecs::policy::swap_pop>
		// constexpr bool erase(std::span<handle_type> ents) { }

		constexpr void erase_at(std::size_t ind) { }

		// template<typename exec_T=ecs::policy::swap_pop>
		// constexpr bool erase_at(std::size_t ind, std::size_t n) { }

		constexpr void clear() {
			// invoke terminate event
			/*
			if constexpr (event_terminate_enabled) {
				if constexpr (requires { reg->template get_resource<const storage_type>(); }) {
				
				
				} else {
					
				}


				if constexpr (storage_enabled) { // ERROR: Use of undeclared identifier 'storage_enabled'
					auto& manager = reg->template get_resource<manager_type>();
					auto& storage = reg->template get_resource<storage_type>();
					
					auto invoker = reg->template on<event::terminate<T>>();
					for (std::size_t pos = 0; pos < size(); ++pos) {
						invoker.invoke(manager[pos], storage[pos]);
					}
				} else {
					auto& manager = reg->template get_resource<manager_type>();
					
					auto invoker = reg->template on<event::terminate<T>>();
					for (std::size_t pos = 0; pos < size(); ++pos) {
						invoker.invoke(manager[pos]);
					}
				}
			}
			*/
			
			reg->template get_resource<manager_type>().clear();
			reg->template get_resource<indexer_type>().clear();
			reg->template get_resource<storage_type>().clear();
		}

		void sync() {
			// check last component if in rope
			// iterate from the back
			// select if erase or insert
			// iterate through updated index lookups
			
			// std::size_t curr_ind = index_of(curr);
			// handle_type& next = manager.at(curr_ind);
			// std::size_t next_ind = index_of(next);

			// std::swap(curr, next);
			


		}

	private:
		reg_T* reg;
	};
}
