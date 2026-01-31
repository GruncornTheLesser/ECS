#pragma once
#include "fwd.h"
#include "traits.h"
#include "registry.h"

namespace ecs {
	template<typename ... Ts>
	class view {
	public:
		using traits = iter_traits<iter<Ts...>>;
		using primary_type = std::tuple_element_t<traits::primary_mixin_index, std::tuple<Ts...>>;

		using iterator = iter<Ts...>;
		using sentinel = iter<Ts...>::sentinel;

		view(registry<>& reg) : reg(reg) { }

		constexpr iterator begin() noexcept { return { this, 0 }; }
		constexpr sentinel end() noexcept { return { this, reg.template pool<typename traits::primary_mixin>().size() }; }
		
		template<typename T>
		T** data() { 
			return pool<T>().template data<T>();
		}

		template<typename T>
		auto& pool() {
			if constexpr (
				std::is_same_v<std::remove_const_t<T>, entity> || 
				std::is_same_v<std::remove_const_t<T>, indirect>
			) {
				return reg.template pool<primary_type>();
			} else {
				return reg.template pool<T>();
			}
		}

		std::size_t size() { 
			return reg.template pool<primary_type>().size();
		}
	private:
		registry<>& reg;
	};
}

