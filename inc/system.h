#pragma once
#include "fwd.h"
#include "registry.h"

namespace ecs {
	template<typename ... Ts>
	class view {
	public:
		using iterator = iter<std::remove_reference_t<Ts>...>;
		using sentinel = iter<std::remove_reference_t<Ts>...>::sentinel;

		using traits = ecs::iter_traits<iterator>;
		using primary_type = typename traits::primary_mixin;


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

		template<typename system_T>
		auto visit(system_T&& visitor) {
			auto it = this->begin();
			auto end = this->end();
			
			while (it != end) {
				auto ref = *it++;
				using ref_type = decltype(ref);
				
				[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
					visitor([&]->Ts {
					 	if constexpr (Is != -1) return get<Is>(ref);
					 	else return Ts{};
					 }()...);
				}(typename traits::exposed_sequence{});
			}
		}
	private:
		registry<>& reg;
	};
}
