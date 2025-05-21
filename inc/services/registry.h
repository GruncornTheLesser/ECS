#pragma once
#include "core/traits.h"

/* 
resources: factory, storage, indexer, manager, invoker
services: pool, view, generator
*/

namespace ecs {
	template<traits::resource_class T>
	struct cache { 
		using value_type = traits::resource::get_value_t<T>;
		using mutex_type = traits::resource::get_mutex_t<T>;

		template<typename Reg_T> requires(std::is_constructible_v<value_type, Reg_T&>)
		cache(Reg_T& reg) : resource(reg) { }

		template<typename Reg_T> requires(!std::is_constructible_v<value_type, Reg_T&>)
		cache(Reg_T& reg) { }

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

		[[no_unique_address]] mutex_type mutex;
		[[no_unique_address]] value_type resource;
	};


	template<typename ... Ts>
	class registry {
	public:
		using resource_set = traits::dependencies::get_resource_set_t<Ts...>;
		using entity_set = traits::dependencies::get_entity_set_t<Ts...>;
		using component_set = traits::dependencies::get_component_set_t<Ts...>;
		using event_set = traits::dependencies::get_event_set_t<Ts...>;

		using resource_cache = util::eval_each_t<resource_set, util::wrap_<cache>::template type>;
	
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

		template<traits::resource_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline cache<T>& get_cache() {
			return std::get<std::remove_const_t<cache<T>>>(resources);
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<const registry<Ts...>, std::add_const_t<T>>)
		inline const cache<T>& get_cache() const {
			return std::get<std::remove_const_t<cache<T>>>(resources);
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::traits::resource::get_value_t<T>& get_resource() {
			return get_cache<T>().resource;
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<const registry<Ts...>, std::add_const_t<T>>)
		inline const ecs::traits::resource::get_value_t<T>& get_resource() const {
			return get_cache<T>().resource;
		}

		template<traits::event_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::invoker<T, registry<Ts...>> on() {
			return this;
		}

		template<traits::event_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline const ecs::invoker<T, const registry<Ts...>> on() const {
			return this;
		}

		template<traits::entity_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::generator<T, registry<Ts...>> generator() {
			return this;
		}

		template<traits::entity_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::generator<T, const registry<Ts...>> generator() const {
			return this;
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::pool<T, registry<Ts...>> pool() {
			return this;
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline ecs::pool<const T, const registry<Ts...>> pool() const {
			return this;
		}
		
		// view
		template<typename ... select_Ts, 
			typename from_T=typename traits::view_builder<select<select_Ts...>>::from_type, 
			typename where_T=typename traits::view_builder<select<select_Ts...>, from_T>::where_type>
		inline ecs::view<ecs::select<select_Ts...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return this;
		}

		template<typename ... select_Ts, 
			typename from_T=typename traits::view_builder<select<select_Ts...>>::from_type, 
			typename where_T=typename traits::view_builder<select<select_Ts...>, from_T>::where_type>
		inline ecs::view<ecs::select<const select_Ts...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return this;
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<registry<Ts...>, T> && std::is_constructible_v<traits::component::get_value_t<T>, arg_Ts...>)
		inline decltype(auto) emplace(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.emplace(ent, std::forward<arg_Ts>(args)...);
		}

		template<traits::component_class comp_T, traits::entity_class ent_T=entity> 
			requires(traits::is_accessible_v<registry<Ts...>, comp_T>)
		inline void erase(traits::entity::get_handle_t<ent_T> ent) {
			pool<comp_T>().erase(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline std::size_t count() const {
			return pool<T>().size();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline void clear() {
			pool<T>().clear();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline bool has_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.contains(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline T& get_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.get_component(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<registry<Ts...>, T>)
		inline const T& get_component(traits::entity::get_handle_t<traits::component::get_entity_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.get(ent);
		}

		template<traits::entity_class ent_T=entity>
		inline traits::entity::get_handle_t<ent_T> create() {
			return generator<ent_T>().create();
		}
		
		template<traits::entity_class ent_T=entity>
		inline void destroy(traits::entity::get_handle_t<ent_T> ent) {
			generator<ent_T>().destroy(ent);
		}

		template<traits::entity_class ent_T=entity>
		inline bool alive(traits::entity::get_handle_t<ent_T> ent) const {
			return generator<ent_T>().alive(ent);
		}

		template<typename ... dep_Ts> requires((traits::is_accessible_v<registry<Ts...>, dep_Ts> && ...))
		void acquire(priority p = priority::MEDIUM) {
			using dep_resource_set = util::eval_t<traits::dependencies::get_resource_set_t<dep_Ts...>,  
				util::sort_by_<traits::resource::get_lock_priority, util::get_type_name>::template type
			>;
			
			util::apply<dep_resource_set>([&]<typename ... Res_Ts>{ 
				(get_cache<Res_Ts>().lock(p), ...); 
			});
		}

		template<typename ... dep_Ts> requires((traits::is_accessible_v<registry<Ts...>, dep_Ts> && ...))
		void release() {
			using dep_resource_set = util::eval_t<traits::dependencies::get_resource_set_t<dep_Ts...>,  
				util::sort_by_<traits::resource::get_lock_priority, util::get_type_name>::template type,
				util::reverse
			>;

			util::apply<dep_resource_set>([&]<typename ... Res_Ts>{
				(get_cache<Res_Ts>().unlock(), ...); 
			});
		}

	private:
		union { resource_cache resources; };
	};
}