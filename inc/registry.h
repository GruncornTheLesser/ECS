#pragma once
#include "traits.h"
#include "generator.h"
#include "pipeline.h"
#include "pool.h"
#include "view.h"
/* 
resources: factory, storage, indexer, manager, invoker
services: pool, view, generator, pipeline
*/

namespace ecs {
	template<typename ... Ts>
	class registry {
	public:
		using resource_set = traits::get_resource_dependencies_t<Ts...>;
		using event_set = traits::get_event_dependencies_t<Ts...>;
		using handle_set = traits::get_table_dependencies_t<Ts...>;
		using component_set = traits::get_component_dependencies_t<Ts...>;
		
		using resource_cache = meta::each_t<resource_set, traits::get_container>;

		registry() = default;
		~registry() {
			meta::apply<component_set>([&]<typename T>(){ ecs::pool<T, registry<Ts...>>{ this }.clear(); });
			meta::apply<handle_set>([&]<typename T>(){ ecs::generator<T, registry<Ts...>>{ this }.clear(); });
		}
		registry(const registry&) = delete;
		registry& operator=(const registry&) = delete;
		registry(registry&&) = default;
		registry& operator=(registry&&) = default;

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& get_resource() {
			return resource_traits<T>::get_resource(std::get<traits::get_container_t<T>>(resources));
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T& get_resource() const {
			return resource_traits<T>::get_resource(std::get<traits::get_container_t<T>>(resources));
		}

		template<traits::event_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline traits::get_invoker_t<T>& on() {
			return get_resource<traits::get_invoker_t<T>>();
		}

		template<traits::event_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const traits::get_invoker_t<T>& on() const {
			return get_resource<traits::get_invoker_t<T>>();
		}

		template<traits::table_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::generator<T, registry<Ts...>> generator() {
			return this;
		}

		template<traits::table_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
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
		requires(meta::conjunction_v<traits::get_resource_dependencies_t<Us...>, ecs::traits::is_accessible, ecs::registry<Ts...>>)
		inline ecs::view<ecs::select<Us...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return this;
		}

		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
		requires(meta::conjunction_v<traits::get_resource_dependencies_t<Us...>, ecs::traits::is_accessible, ecs::registry<Ts...>>)
		inline ecs::view<ecs::select<const Us...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return this;
		}

		template<typename ... Us> requires(meta::conjunction_v<traits::get_resource_dependencies_t<Us...>, ecs::traits::is_accessible, ecs::registry<Ts...>>)
		inline ecs::pipeline<traits::get_resource_dependencies_t<Us...>, ecs::registry<Ts...>> 
		pipeline(ecs::priority p = priority::MEDIUM) {
			return { this, p };
		}

		template<typename ... Us> requires(meta::conjunction_v<traits::get_resource_dependencies_t<Us...>, ecs::traits::is_accessible, ecs::registry<Ts...>>)
		inline ecs::pipeline<meta::concat_t<traits::get_resource_dependencies_t<Us>...>, const ecs::registry<Ts...>> 
		pipeline(ecs::priority p = priority::MEDIUM) const {
			return { this, p };
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_get(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.try_get(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& try_emplace(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_emplace(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& emplace(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.emplace(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void erase(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.erase(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool try_erase(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_erase(ent);
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
		inline bool has_component(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.contains(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& get_component(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.get_component(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T& get_component(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.get(ent);
		}

		template<traits::table_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline traits::get_handle_t<T> create() {
			return generator<T>().create();
		}
		
		template<traits::table_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void destroy(traits::get_handle_t<T> ent) {
			generator<T>().destroy(ent);
		}

		template<traits::table_class T=ecs::entity> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool alive(traits::get_handle_t<T> ent) const {
			return generator<T>().alive(ent);
		}

		template<traits::resource_class ... Us> requires((traits::is_accessible_v<Us, registry<Ts...>> && ...))
		void acquire(priority p = priority::MEDIUM) {
			meta::apply<meta::sort_by_t<std::tuple<Us...>, meta::get_typeID>>([&]<typename U>{
				resource_traits<U>::acquire(std::get<traits::get_container_t<U>>(resources), p);
			});
		}

		template<traits::resource_class ... Us> requires((traits::is_accessible_v<Us, registry<Ts...>> && ...))
		void release() {
			meta::apply<meta::sort_by_t<std::tuple<Us...>, meta::get_typeID>>([&]<typename U>{
				resource_traits<U>::release(std::get<traits::get_container_t<U>>(resources));
			});
		}

	private:
		resource_cache resources;
	};
}