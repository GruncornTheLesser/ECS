#pragma once
#include "core/fwd.h"

namespace ecs {
	template<typename T, typename reg_T>
	class generator {
	public:
		using registry_type = reg_T;
		using entity_type = T;
		using handle_type = typename entity_traits<T>::handle_type;
		using factory_type = typename entity_traits<T>::factory_type;
		
		using value_type = typename handle_traits<handle_type>::value_type;
		using version_type = typename handle_traits<handle_type>::version_type;
		using integral_type = typename handle_traits<handle_type>::integral_type;

		inline constexpr generator(reg_T* reg) noexcept : reg(reg) { }
		
		constexpr handle_type create() {
			auto& fact = reg->template get_resource<factory_type>();
			
			if (fact.inactive == tombstone{ }) {
				return fact.active.emplace_back(handle_type{ static_cast<integral_type>(fact.active.size()) });
			}

			// get head of inactive list
			handle_type tmp = fact.active[fact.inactive];
			
			// resurrect previous handle
			handle_type hnd = handle_type{ fact.inactive, ++version_type{ tmp } };
			fact.active[fact.inactive] = hnd;
			
			// iterate inactive list
			fact.inactive = value_type{ tmp };
			
			// invoke create event
			if constexpr (traits::is_accessible_v<event::create<entity_type>, reg_T>) {
				reg->template on<event::create<entity_type>>().invoke(hnd);
			}
			
			return hnd;
		}
		
		constexpr void destroy(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();

			// get handle value
			std::size_t pos = value_type{ hnd }; 
			
			// if not alive
			if (fact.active[pos] != hnd) return; 
			
			// invoke destroy event
			if constexpr (traits::is_accessible_v<event::destroy<entity_type>, reg_T>) {
				reg->template on<event::destroy<entity_type>>().invoke(hnd);
			}

			fact.active[pos] = handle_type{ fact.inactive, version_type{ hnd } };
			fact.inactive = pos;
		}

		constexpr bool alive(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();
			return hnd == fact.active[value_type{ hnd }];
		}
	
		constexpr void clear() {
			auto& fact = reg->template get_resource<factory_type>();
			
			if constexpr (traits::is_accessible_v<event::destroy<entity_type>, reg_T>)
			{
				for (std::size_t i = 0; i < fact.active.size(); ++i)
				{
					handle_type hnd = fact.active[i];
					if (static_cast<integral_type>(value_type{ hnd }) == i) {
						reg->template on<event::destroy<entity_type>>().invoke(hnd);
					}
				}
			}

			fact.active.clear();
			fact.inactive = value_type{ tombstone{ } };
		}
	private:
		reg_T* reg;
	};
}