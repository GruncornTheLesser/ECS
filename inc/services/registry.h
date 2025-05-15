#pragma once
#include "core/traits.h"

/* 
resources: factory, storage, indexer, manager, invoker
services: pool, view, generator
*/

namespace ecs {
	template<ecs::traits::resource_class T>
	struct cache { 
		using value_type = ecs::traits::resource::get_value_t<T>;
		using mutex_type = ecs::traits::resource::get_mutex_t<T>;

		template<typename Reg_T> requires(std::is_constructible_v<value_type, Reg_T&>)
		cache(Reg_T& reg) : resource(reg) { }

		template<typename Reg_T> requires(!std::is_constructible_v<value_type, Reg_T&>)
		cache(Reg_T& reg) : resource() { }

		void lock() { 
			if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().lock())>) {
				mutex.lock();
			}
		}
		void lock() const { 
			if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().shared_lock())>) {
				mutex.shared_lock();
			} else if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().lock())>) {
				mutex.lock();
			}
		}
		void unlock() { 
			if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().lock())>) {
				mutex.unlock();
			}
		}
		void unlock() const {
			if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().shared_unlock())>) {
				mutex.shared_unlock();
			} else if constexpr (std::is_void_v<decltype(std::declval<mutex_type>().unlock())>) {
				mutex.unlock();
			}
		}

		[[no_unique_address]] value_type resource;
		[[no_unique_address]] mutex_type mutex;
	};


	template<typename ... Ts>
	class registry {
	public:
		using resource_set = traits::get_resource_dependencies_t<Ts...>;
		using entity_set = traits::get_entity_dependencies_t<Ts...>;
		using component_set = traits::get_component_dependencies_t<Ts...>;
		using resource_cache = util::eval_each_t<resource_set, traits::resource::get_cache>;
	
	public:
		registry() {
			using init_priority = util::sort_by_t<resource_set, traits::resource::get_init_priority>;
			util::apply<init_priority>([&]<typename ... res_Ts>(){ 
				(std::construct_at(&get_cache<res_Ts>(), *this), ...);
			});
		}
		~registry() {
			util::apply<component_set>([&]<typename ... comp_Ts>(){ 
				(pool<comp_Ts>().clear(), ...); 
			});
			util::apply<entity_set>([&]<typename ... ent_Ts>(){ 
				(generator<ent_Ts>().clear(), ...);
			});
			using term_priority = util::reverse_t<util::sort_by_t<resource_set, traits::resource::get_init_priority>>;
			util::apply<term_priority>([&]<typename ... res_Ts>(){ 
				(std::destroy_at(&get_resource<res_Ts>()), ...);
			});
		}

		registry(const registry&) = delete;
		registry& operator=(const registry&) = delete;
		registry(registry&& other) = delete;
		registry& operator=(registry&& other) = delete;

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline traits::resource::get_cache_t<T>& get_cache() {
			return std::get<std::remove_const_t<traits::resource::get_cache_t<T>>>(resources);
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const traits::resource::get_cache_t<T>& get_cache() const {
			return std::get<std::remove_const_t<traits::resource::get_cache_t<T>>>(resources);
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::traits::resource::get_value_t<T>& get_resource() {
			return get_cache<T>().resource;
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const ecs::traits::resource::get_value_t<T>& get_resource() const {
			return get_cache<T>().resource;
		}

		template<traits::event_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::invoker<T, registry<Ts...>> on() {
			return this;
		}

		template<traits::event_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const ecs::invoker<T, const registry<Ts...>> on() const {
			return this;
		}

		template<traits::entity_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::generator<T, registry<Ts...>> generator() {
			return this;
		}

		template<traits::entity_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::generator<T, const registry<Ts...>> generator() const {
			return this;
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::pool<T, registry<Ts...>> pool() {
			return this;
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::pool<const T, const registry<Ts...>> pool() const {
			return this;
		}
		
		// view
		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
		requires(util::pred::allof_v<traits::get_resource_dependencies_t<Us...>, util::pred_<ecs::traits::is_accessible, ecs::registry<Ts...>>::template type>)
		inline ecs::view<ecs::select<Us...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return this;
		}

		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
			requires(util::pred::allof_v<traits::get_resource_dependencies_t<Us...>, util::pred_<ecs::traits::is_accessible, ecs::registry<Ts...>>::template type>)
		inline ecs::view<ecs::select<const Us...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return this;
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T* try_get(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent, arg_Ts&& ... args) {
			ecs::pool<const T, registry<Ts...>> pl{ this };
			std::size_t ind = pl.index_of(ent);
			return ind == -1 ? ind : pl.at(ind);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T* try_get(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) const {
			ecs::pool<const T, const registry<Ts...>> pl{ this };
			std::size_t ind = pl.index_of(ent);
			return ind == -1 ? ind : pl.at(ind);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>> && std::is_constructible_v<T, arg_Ts...>)
		inline decltype(auto) emplace(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.emplace(ent, std::forward<arg_Ts>(args)...);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void erase(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) {
			ecs::pool<T, registry<Ts...>>{ this }.erase(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline std::size_t count() const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.size();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void clear() {
			pool<std::remove_const_t<T>>().clear();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool has_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.contains(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& get_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.get_component(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T& get_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.get(ent);
		}

		template<traits::entity_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline traits::entity::get_handle_t<T> create() {
			return generator<T>().create();
		}
		
		template<traits::entity_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void destroy(traits::entity::get_handle_t<T> ent) {
			generator<T>().destroy(ent);
		}

		template<traits::entity_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool alive(traits::entity::get_handle_t<T> ent) const {
			return generator<T>().alive(ent);
		}

		template<typename ... Us> requires((traits::is_accessible_v<Us, registry<Ts...>> && ...))
		void acquire(priority p = priority::MEDIUM) {
			util::apply<util::sort_by_t<traits::get_resource_dependencies_t<Us...>, 
				traits::resource::get_lock_priority, util::get_type_name>>([&]<typename ... Res_Ts>{ 
				(get_cache<Res_Ts>().lock(), ...); 
			});
		}

		template<typename ... Us> requires((traits::is_accessible_v<Us, registry<Ts...>> && ...))
		void release() {
			util::apply<util::reverse_t<util::sort_by_t<traits::get_resource_dependencies_t<Us...>, 
				traits::resource::get_lock_priority, util::get_type_name>>>([&]<typename ... Res_Ts>{
				(get_cache<Res_Ts>().unlock(), ...); 
			});
		}

	private:
		union { resource_cache resources; };
	};
}