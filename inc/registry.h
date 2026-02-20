#pragma once
#include "fwd.h"
#include <memory>
#include <unordered_map>

// registry.h
namespace ecs {
	template<typename ... Ts> class resource_cache;

	template<>
	class resource_cache<> {
		template<typename U> 
		static constexpr void erased_deleter(void* ptr) {
			std::destroy_at(static_cast<U*>(ptr));
			std::allocator<U>{}.deallocate(static_cast<U*>(ptr), 1);
		}
		
		using erased_ptr = std::unique_ptr<void, void(*)(void*)>;
		using data_container = std::unordered_map<ecs::id, erased_ptr>;
	public:
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr U& cache();
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr const U& cache() const;
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr void drop();
	private:
		data_container data;
	};

	template<typename ... Ts>
	class resource_cache : public resource_cache<> {
		template<id key>
		static constexpr bool local_index = []{ 
			std::size_t i = -1;
			((++i, key == resource<Ts>::id) || ...);
			return i;
		}();
		
	public:
		constexpr resource_cache();
		template<typename ... Us> constexpr resource_cache(resource_cache<Us...>&& other);
		template<typename ... Us> constexpr resource_cache& operator=(resource_cache<Us...>&& other);
		
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr U& cache();
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr const U& cache() const;
		template<typename U, ecs::id key=std::type_identity<std::remove_const_t<U>>{}> 
		constexpr void drop();
	private:
		std::tuple<typename resource<Ts>::type*...> data;
	};
}

template<typename ... Ts>
constexpr ecs::resource_cache<Ts...>::resource_cache()
 : resource_cache<>(), data(std::addressof(resource_cache<>::template cache<Ts, resource<Ts>::id>())...) { }

template<typename ... Ts>
template<typename ... Us>
constexpr ecs::resource_cache<Ts...>::resource_cache(resource_cache<Us...>&& other)
 : resource_cache<>(std::move(other)), data(std::addressof(resource_cache<>::template cache<Ts, resource<Ts>::id>())...) { }

template<typename ... Ts>
template<typename ... Us>
constexpr ecs::resource_cache<Ts...>& ecs::resource_cache<Ts...>::operator=(resource_cache<Us...>&& other) { 
	resource_cache<>::operator=(std::move(other));
	data = { std::addressof(resource_cache<>::template cache<Ts, resource<Ts>::id>())... };
	return *this;
}

template<typename T, ecs::id key>
constexpr T& ecs::resource_cache<>::cache() {
	using value_type = std::remove_const_t<T>;

	auto it = data.find(key);
	if (it != data.end()) {
		return *static_cast<value_type*>(it->second.get());
	}

	value_type* ptr = std::allocator<value_type>{}.allocate(1);
	std::construct_at(ptr);
	data.emplace_hint(it, key, erased_ptr{ ptr, erased_deleter<value_type> });

	return *ptr;
}

template<typename T, ecs::id key>
constexpr const T& ecs::resource_cache<>::cache() const {
	return *static_cast<const T*>(data.at(key).get());
}

template<typename T, ecs::id key>
constexpr void ecs::resource_cache<>::drop() {
	if (auto it = data.find(key); it != data.end()) {
		data.erase(it);
	}
}

template<typename ... Ts>
template<typename U, ecs::id key>
constexpr U& ecs::resource_cache<Ts...>::cache() {
	if constexpr (local_index<key> != sizeof...(Ts)) { 
		U*& ptr = std::get<local_index<key>>(data);
		if (ptr == nullptr) {
			ptr = std::addressof(resource_cache<>::template cache<U, key>());
		}
		return *ptr;
	} 
	else {
		return resource_cache<>::template cache<U, key>();
	}
}
template<typename ... Ts>
template<typename U, ecs::id key> 
constexpr const U& ecs::resource_cache<Ts...>::cache() const {
	if constexpr (local_index<key> != sizeof...(Ts)) { 
		U*& ptr = std::get<local_index<key>>(data);
		if (ptr == nullptr) {
			ptr = std::addressof(resource_cache<>::template cache<U, key>());
		}
		return *ptr;
	} 
	else {
		return resource_cache<>::template cache<U, key>();
	}
}

template<typename ... Ts>
template<typename U, ecs::id key>
constexpr void ecs::resource_cache<Ts...>::drop() {
	if constexpr (local_index<key> != sizeof...(Ts)) { 
		std::get<local_index<key>>(data) = nullptr;
	} 
	return resource_cache<>::template drop<U, key>();
}

namespace ecs {
	template<typename ... Ts>
	class registry : public resource_cache<Ts...> {
	public:
		// pool
		template<typename U> auto& pool();
		template<typename U> const auto& pool() const;

		// view
		template<typename ... Us> ecs::view<Us...> view();
		template<typename ... Us> ecs::view<const Us...> view() const;

		// entity
		[[nodiscard]] constexpr entity create_entity();
		constexpr void destroy_entity(const entity& ent);
		[[nodiscard]] constexpr bool entity_alive(const entity& ent) const;

		// component
		template<typename U, typename ... arg_Us> U& add_component(const entity& ent, arg_Us&&... args);
		template<typename U, typename ... arg_Us> U& add_component(const ecs::pool<U>::const_iterator& it, const entity& ent, arg_Us&&... args);
		template<typename U> void remove_component(const entity& ent);
		template<typename U> U& get_component(const entity& ent);
		template<typename U> [[nodiscard]] constexpr bool has_component(const entity& ent) const;
		
		template<typename system_T>
		void visit(system_T&& visitor);
	};
}

template<typename ... Ts>
template<typename T>
auto& ecs::registry<Ts...>::pool() {
	return resource_cache<Ts...>::template cache<std::conditional_t<std::is_const_v<T>, const ecs::pool<std::remove_const_t<T>>, ecs::pool<T>>>();
}

template<typename ... Ts>
template<typename T>
const auto& ecs::registry<Ts...>::pool() const {
	return resource_cache<Ts...>::template cache<const ecs::pool<std::remove_const_t<T>>>();
}


template<typename ... Ts>
template<typename ... Us>
ecs::view<Us...> ecs::registry<Ts...>::view() {
	return { *this };
}
template<typename ... Ts>
template<typename ... Us>
ecs::view<const Us...> ecs::registry<Ts...>::view() const {
	return { *this };
}

template<typename ... Ts>
[[nodiscard]] constexpr ecs::entity ecs::registry<Ts...>::create_entity() {
	return pool<entity>().create();
}

template<typename ... Ts>
constexpr void ecs::registry<Ts...>::destroy_entity(const entity& ent) {
	return pool<entity>().destroy(ent);
}

template<typename ... Ts>
[[nodiscard]] constexpr bool ecs::registry<Ts...>::entity_alive(const entity& ent) const {
	return pool<entity>().alive(ent);
}

template<typename ... Ts>
template<typename U, typename ... arg_Us>
U& ecs::registry<Ts...>::add_component(const entity& ent, arg_Us&&... args) {
	return pool<U>().emplace_back(ent, std::forward<arg_Us>(args)...);
}

template<typename ... Ts>
template<typename U, typename ... arg_Us>
U& ecs::registry<Ts...>::add_component(const ecs::pool<U>::const_iterator& it, const entity& ent, arg_Us&&... args) {
	return pool<U>().template emplace(it, ent, std::forward<arg_Us>(args)...);
}

template<typename ... Ts>
template<typename U>
void ecs::registry<Ts...>::remove_component(const entity& ent) {
	pool<U>().template erase(ent);
}

template<typename ... Ts>
template<typename T>
T& ecs::registry<Ts...>::get_component(const entity& ent) {
	return *pool<T>().find(ent);
}

template<typename ... Ts>
template<typename T>
[[nodiscard]] constexpr bool ecs::registry<Ts...>::has_component(const entity& ent) const {
	return pool<T>().contains(ent);
}

template<typename ... Ts>
template<typename system_T>
void ecs::registry<Ts...>::visit(system_T&& visitor) {
	using sys_traits = typename ecs::system_traits<system_T>;
	
	typename sys_traits::view_type view(*this);
	return view.visit(std::forward<system_T>(visitor));
}