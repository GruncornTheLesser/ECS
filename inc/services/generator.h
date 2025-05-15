#pragma once
#include "core/traits.h"

namespace ecs {
	template<ecs::traits::entity_class T, typename reg_T>
	class generator {
	public:
		using registry_type = reg_T;
		using entity_type = T;
		using handle_type = typename entity_traits<T>::handle_type;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
		using integral_type = typename handle_type::integral_type;
		using factory_type = typename entity_traits<T>::factory_type;
		
		inline constexpr generator(reg_T* reg) noexcept : reg(reg) { }
		
		constexpr handle_type create() {
			auto& fact = reg->template get_resource<factory_type>();
			
			if (fact.inactive == tombstone{ }) {
				return fact.active.emplace_back(handle_type{ static_cast<integral_type>(fact.active.size()) });
			}

			// get head of inactive list
			integral_type next = value_view{ fact.inactive };
			handle_type tmp = fact.active[next];
			
			// resurrect previous handle
			handle_type hnd = handle_type{ next, ++version_view{ tmp } };
			fact.active[next] = hnd;
			
			// iterate inactive list
			fact.inactive = value_view{ tmp };
			
			// invoke create event
			if constexpr (traits::is_accessible_v<event::create<entity_type>, reg_T>) {
				reg->template on<event::create<entity_type>>().invoke(hnd);
			}
			
			return hnd;
		}
		
		constexpr void destroy(handle_type ent) {
			auto& fact = reg->template get_resource<factory_type>();
			
			// get handle value
			integral_type pos = value_view{ ent }; 
			
			// if not alive
			if (fact.active[pos] != ent) return; 

			// invoke destroy event
			if constexpr (traits::is_accessible_v<event::destroy<entity_type>, reg_T>) {
				reg->template on<event::destroy<entity_type>>().invoke(ent);
			}
			
			// immediate-mode
			using entity_component_set = ecs::meta::filter_t<typename registry_type::component_set, traits::is_entity_match, T>;
			ecs::meta::apply<entity_component_set>([&]<typename ... Ts>() { (reg->template pool<Ts>().erase(ent), ...); });
			
			fact.active[pos] = handle_type{ fact.inactive, version_view{ ent } };
			fact.inactive = ent;
		}

		constexpr bool alive(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();
			return hnd == fact.active[value_view{ hnd }];
		}
	
		constexpr void clear() {
			auto& fact = reg->template get_resource<factory_type>();
			
			if constexpr (traits::is_accessible_v<event::destroy<entity_type>, reg_T>)
			{
				for (std::size_t i = 0; i < fact.active.size(); ++i)
				{
					handle_type hnd = fact.active[i];
					if (static_cast<integral_type>(value_view{ hnd }) == i) {
						reg->template on<event::destroy<entity_type>>().invoke(hnd);
					}
				}
			}

			fact.active.clear();
			fact.inactive = tombstone{ };
		}
	private:
		reg_T* reg;
	};
}