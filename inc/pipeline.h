#pragma once
#include "traits.h"

namespace ecs {
    template<template<typename...> typename set_T, typename ... Ts, typename reg_T>
    class pipeline<set_T<Ts...>, reg_T> {
    public:
        pipeline(reg_T* reg, priority p) : reg(reg) {
            reg->template acquire<Ts...>(p);
        }
        ~pipeline() {
            reg->template release<Ts...>();
        }
        pipeline(pipeline&& other) = delete;
        pipeline& operator=(pipeline&& other) = delete;
        pipeline(const pipeline& other) = delete;
        pipeline& operator=(const pipeline& other) = delete;

        template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& get_resource() {
			return reg->template get_resource<T>();
		}

		template<traits::resource_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T& get_resource() const {
			return const_cast<const reg_T*>(reg)->template get_resource<T>();
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
			return reg;
		}

		template<traits::table_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::generator<T, const registry<Ts...>> generator() const {
			return const_cast<const reg_T*>(reg);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::pool<T, registry<Ts...>> pool() {
			return reg;
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline ecs::pool<const T, const registry<Ts...>> pool() const {
			return const_cast<const reg_T*>(reg);
		}
		
		// view
		template<typename ... Us,
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
        requires (true)
		inline ecs::view<ecs::select<Us...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return reg;
		}

		template<typename ... Us, 
			typename from_T=typename traits::view_builder<select<Us...>>::from_type, 
			typename where_T=typename traits::view_builder<select<Us...>, from_T>::where_type>
		requires (true)
        inline ecs::view<ecs::select<const Us...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return const_cast<const reg_T*>(reg);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return pool<T>()->try_get(ent, args...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T* try_get(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) const {
			return pool<T>()->try_get(ent, args...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& try_emplace(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return pool<T>().try_emplace(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T, typename ... arg_Ts> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& emplace(traits::get_handle_t<traits::get_table_t<T>> ent, arg_Ts&& ... args) {
			return pool<T>().emplace(ent, std::forward<arg_Ts>()...);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void erase(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return pool<T>().erase(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool try_erase(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return pool<T>().try_erase(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline std::size_t count() const {
			return pool<T>().size();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline void clear() {
			pool<T>().clear();
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline bool has_component(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return pool<T>().contains(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline T& get_component(traits::get_handle_t<traits::get_table_t<T>> ent) {
			return pool<T>().get_component(ent);
		}

		template<traits::component_class T> requires(traits::is_accessible_v<T, registry<Ts...>>)
		inline const T& get_component(traits::get_handle_t<traits::get_table_t<T>> ent) const {
			return pool<T>().get(ent);
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


    private:
        reg_T* reg;
    };
}