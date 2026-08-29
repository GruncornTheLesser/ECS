#pragma once
#include <memory>
#include <unordered_map>
#include "fwd.h"
#include "traits.h"

namespace ecs {
	template<>
	class res_cache<> {
		template<typename U> 
		static constexpr void erased_deleter(void* ptr) {
			std::destroy_at(static_cast<U*>(ptr));
			std::allocator<U>{}.deallocate(static_cast<U*>(ptr), 1);
		}
		
		using erased_ptr = std::unique_ptr<void, void(*)(void*)>;
		using data_container = std::unordered_map<label, erased_ptr>;
	public:
		template<typename U> constexpr void drop();
		template<typename U> constexpr typename res_traits<U>::type& cache();
		template<typename U> constexpr const typename res_traits<U>::type& cache() const;
	private:
		data_container data;
	};

	template<typename ... Ts>
	class res_cache : public res_cache<> {
	public:
		template<label key>
		static constexpr std::size_t local_index = []->std::size_t { 
			if (std::size_t i = -1; ((++i, key == res_traits<Ts>::key) || ...)) return i;
			return -1;
		}();
		
		constexpr res_cache();
		
		template<typename ... Us> constexpr res_cache(res_cache<Us...>&& other);
		template<typename ... Us> constexpr res_cache& operator=(res_cache<Us...>&& other);
		
		template<typename U> constexpr typename res_traits<U>::type& cache();
		template<typename U> constexpr const typename res_traits<U>::type& cache() const;
		template<typename U> constexpr void drop();
	private:
		std::tuple<typename res_traits<Ts>::type*...> data;
	};
}

template<typename ... Ts>
constexpr ecs::res_cache<Ts...>::res_cache()
 : res_cache<>(), data(std::addressof(res_cache<>::template cache<Ts>())...) { }

template<typename ... Ts>
template<typename ... Us>
constexpr ecs::res_cache<Ts...>::res_cache(res_cache<Us...>&& other)
 : res_cache<>(std::move(other)), data(std::addressof(res_cache<>::template cache<Ts>())...) { }

template<typename ... Ts>
template<typename ... Us>
constexpr ecs::res_cache<Ts...>& ecs::res_cache<Ts...>::operator=(res_cache<Us...>&& other) { 
	res_cache<>::operator=(std::move(other));
	data = { std::addressof(res_cache<>::template cache<Ts>())... };
	return *this;
}

template<typename U>
constexpr void ecs::res_cache<>::drop() {
	if (auto it = data.find(res_traits<U>::key); it != data.end()) {
		data.erase(it);
	}
}

template<typename U>
constexpr typename ecs::res_traits<U>::type& ecs::res_cache<>::cache() {
	auto it = data.find(res_traits<U>::key);
	if (it != data.end()) {
		return *static_cast<typename res_traits<U>::type*>(it->second.get());
	}

	typename res_traits<U>::type* ptr = std::allocator<typename res_traits<U>::type>{}.allocate(1);
	std::construct_at(ptr);
	data.emplace_hint(it, res_traits<U>::key, erased_ptr{ ptr, erased_deleter<typename res_traits<U>::type> });

	return *ptr;
}

template<typename U>
constexpr const typename ecs::res_traits<U>::type& ecs::res_cache<>::cache() const {
	static constexpr label res_key = U::key;
	return *static_cast<const typename res_traits<U>::type*>(data.at(res_key).get());
}

template<typename ... Ts>
template<typename U>
constexpr typename ecs::res_traits<U>::type& ecs::res_cache<Ts...>::cache() {
	static constexpr std::size_t res_idx = local_index<res_traits<U>::key>;
	
	if constexpr (res_idx != -1) { 
		typename res_traits<U>::type*& ptr = std::get<res_idx>(data);
		if (ptr == nullptr) {
			ptr = std::addressof(res_cache<>::template cache<U>());
		}
		return *ptr;
	} 
	else {
		return res_cache<>::template cache<U>();
	}
}
template<typename ... Ts>
template<typename U> 
constexpr const typename ecs::res_traits<U>::type& ecs::res_cache<Ts...>::cache() const {
	static constexpr std::size_t res_idx = local_index<res_traits<U>::key>;
	
	if constexpr (res_idx != -1) { 
		typename res_traits<U>::type*& ptr = std::get<res_idx>(data);
		if (ptr == nullptr) {
			ptr = std::addressof(res_cache<>::template cache<U>());
		}
		return *ptr;
	} 
	else {
		return res_cache<>::template cache<U>();
	}
}

template<typename ... Ts>
template<typename U>
constexpr void ecs::res_cache<Ts...>::drop() {
	static constexpr std::size_t res_idx = local_index<res_traits<U>::key>;
	
	if constexpr (res_idx != -1) {
		std::get<res_idx>(data) = nullptr;
	} 
	return res_cache<>::template drop<U>();
}