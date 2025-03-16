
#pragma once
#include "traits.h"
#include "handle.h"
#include "generator.h"
#include "invoker.h"
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
		using resource_set = meta::unique_t<meta::concat_t<traits::get_resource_dependencies_t<Ts>...>>;
		using event_set = meta::unique_t<meta::concat_t<traits::get_event_dependencies_t<Ts>...>>;
		using handle_set = meta::unique_t<meta::concat_t<traits::get_table_dependencies_t<Ts>...>>;
		using component_set = meta::unique_t<meta::concat_t<traits::get_component_dependencies_t<Ts>...>>;
		
		registry() = default;
		~registry() {
			meta::apply<component_set>([&]<typename T>(){ ecs::pool<T, registry<Ts...>>{ this }.clear(); });
			meta::apply<handle_set>([&]<typename T>(){ ecs::generator<T, registry<Ts...>>{ this }.clear(); });
		}
		registry(const registry&) = delete;
		registry& operator=(const registry&) = delete;
		registry(registry&&) = default;
		registry& operator=(registry&&) = default;

		template<traits::resource_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline T& get_resource() {
			return std::get<std::remove_const_t<T>>(resources);
		}

		template<traits::resource_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline const T& get_resource() const {
			return std::get<std::remove_const_t<T>>(resources);
		}

		template<traits::event_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline traits::get_invoker_t<T>& on() {
			return get_resource<traits::get_invoker_t<T>>();
		}

		template<traits::event_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline const traits::get_invoker_t<T>& on() const {
			return get_resource<traits::get_invoker_t<T>>();
		}

		template<traits::table_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline ecs::generator<T, registry<Ts...>> generator() {
			return this;
		}

		template<traits::table_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline ecs::generator<T, const registry<Ts...>> generator() const {
			return this;
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline ecs::pool<T, registry<Ts...>> pool() {
			return this;
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline ecs::pool<const T, const registry<Ts...>> pool() const {
			return this;
		}
		
		// view
		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
		inline ecs::view<ecs::select<Us...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return this;
		}

		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
		inline ecs::view<ecs::select<const Us...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return this;
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_get(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline const T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.try_get(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline T& try_init(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_init(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline T& init(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return ecs::pool<T, registry<Ts...>>{ this }.init(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline void terminate(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.terminate(ent);
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline bool try_terminate(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.try_terminate(ent);
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline std::size_t count() const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.size();
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline void clear() {
			pool<std::remove_const_t<T>>().clear();
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline bool has(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.contains(ent);
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline T& get(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return ecs::pool<T, registry<Ts...>>{ this }.get(ent);
		}

		template<traits::component_class T> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline const T& get(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return ecs::pool<const T, const registry<Ts...>>{ this }.get(ent);
		}

		template<traits::table_class T=table> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline traits::get_handle_t<T> create() {
			return generator<T>().template create();
		}
		
		template<traits::table_class T=table> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline void destroy(traits::get_handle_t<T> ent) {
			generator<T>().template destroy(ent);
		}

		template<traits::table_class T=table> requires(traits::is_compatible_v<registry<Ts...>, T>)
		inline bool alive(traits::get_handle_t<T> ent) const {
			return generator<T>().template alive(ent);
		}

		template<traits::resource_class ... Us> requires((traits::is_compatible_v<registry<Ts...>, Us> && ...))
		void acquire(priority p = priority::MEDIUM) {
			meta::apply<meta::sort_by_t<std::tuple<Us...>, meta::get_typeID>>([&]<typename T>{
				get_resource<T>().lock(p);
			});
		}

		template<traits::resource_class ... Us> requires((traits::is_compatible_v<registry<Ts...>, Us> && ...))
		void release() {
			meta::apply<meta::sort_by_t<std::tuple<Us...>, meta::get_typeID>>([&]<typename T>{
				get_resource<T>().unlock();
			});
		}

	private:
		resource_set resources;
	};
}
