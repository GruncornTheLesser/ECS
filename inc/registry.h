#pragma once
#include "fwd.h"
#include "resource.h"

// registry.h

namespace ecs {
	template<typename ... Ts>
	class registry : public res_cache<Ts...> {
	public:
		constexpr operator registry<>&();
		constexpr operator const registry<>&() const;

		// pool
		template<typename U> constexpr auto& pool();
		template<typename U> constexpr const auto& pool() const;

		// view
		template<typename ... Us> constexpr ecs::view<Us...> view();
		template<typename ... Us> constexpr ecs::view<const Us...> view() const;

		// entity
		template<typename ... Us>
		[[nodiscard]] constexpr entity create(Us&& ... components);
		[[nodiscard]] constexpr bool alive(entity ent) const;
		constexpr void destroy(entity ent);

		// component
		template<typename U, typename ... arg_Us> constexpr U& add(entity ent, arg_Us&&... args);
		template<typename U, typename ... arg_Us> constexpr U& add(const ecs::pool<U>::const_iterator& it, entity ent, arg_Us&&... args);
		template<typename U> constexpr void remove(entity ent);
		template<typename U> constexpr U& get(entity ent);
		template<typename U> [[nodiscard]] constexpr bool has(entity ent) const;
		
		template<typename U> constexpr void visit(U&& visitor);
	};
}

template<typename ... Ts>
constexpr ecs::registry<Ts...>::operator ecs::registry<>&() {
	return static_cast<registry<>&>(static_cast<res_cache<>&>(*this));
}

template<typename ... Ts>
constexpr ecs::registry<Ts...>::operator const ecs::registry<>&() const {
	return static_cast<const registry<>&>(static_cast<const res_cache<>&>(*this));
}

template<typename ... Ts>
template<typename T>
constexpr auto& ecs::registry<Ts...>::pool() {
	return res_cache<Ts...>::template cache<std::conditional_t<std::is_const_v<T>, const ecs::pool<std::remove_const_t<T>>, ecs::pool<T>>>();
}

template<typename ... Ts>
template<typename U>
constexpr const auto& ecs::registry<Ts...>::pool() const {
	return res_cache<Ts...>::template cache<const ecs::pool<std::remove_const_t<U>>>();
}


template<typename ... Ts>
template<typename ... Us>
constexpr ecs::view<Us...> ecs::registry<Ts...>::view() {
	return { *this };
}
template<typename ... Ts>
template<typename ... Us>
constexpr ecs::view<const Us...> ecs::registry<Ts...>::view() const {
	return { *this };
}

template<typename ... Ts>
template<typename ... Us>
[[nodiscard]] constexpr ecs::entity ecs::registry<Ts...>::create(Us&& ... components) {
	entity handle = pool<entity>().create();
	(pool<Us>().emplace_back(handle, std::move(components)), ...);
	return handle;
}

template<typename ... Ts>
constexpr void ecs::registry<Ts...>::destroy(entity ent) {
	return pool<entity>().destroy(ent);
}

template<typename ... Ts>
[[nodiscard]] constexpr bool ecs::registry<Ts...>::alive(entity ent) const {
	return pool<entity>().alive(ent);
}

template<typename ... Ts>
template<typename U, typename ... arg_Us>
constexpr U& ecs::registry<Ts...>::add(entity ent, arg_Us&&... args) {
	return pool<U>().emplace_back(ent, std::forward<arg_Us>(args)...);
}

template<typename ... Ts>
template<typename U, typename ... arg_Us>
constexpr U& ecs::registry<Ts...>::add(const ecs::pool<U>::const_iterator& it, entity ent, arg_Us&&... args) {
	return pool<U>().emplace(it, ent, std::forward<arg_Us>(args)...);
}

template<typename ... Ts>
template<typename U>
constexpr void ecs::registry<Ts...>::remove(entity ent) {
	pool<U>().erase(ent);
}

template<typename ... Ts>
template<typename U>
constexpr U& ecs::registry<Ts...>::get(entity ent) {
	return *pool<U>().find(ent);
}

template<typename ... Ts>
template<typename U>
[[nodiscard]] constexpr bool ecs::registry<Ts...>::has(entity ent) const {
	return pool<U>().contains(ent);
}

template<typename ... Ts>
template<typename U>
constexpr void ecs::registry<Ts...>::visit(U&& visitor) {
	using sys_traits = typename ecs::system_traits<U>;
	
	typename sys_traits::view_type view(*this);
	return view.visit(std::forward<U>(visitor));
}