#pragma once
#include "core/traits.h"

namespace ecs {
	template<ecs::traits::entity_class T, typename reg_T>
	class generator {
	public:
		using registry_type = reg_T;
		using entity_type = T;
		using handle_type = traits::entity::get_handle_t<T>;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
		using integral_type = typename handle_type::integral_type;
		using factory_type = traits::entity::get_factory_t<T>;
		using create_event = traits::entity::get_create_event_t<T>;
		using destroy_event = traits::entity::get_destroy_event_t<T>;

		inline constexpr generator(reg_T* reg) noexcept : reg(reg) { }
		
		constexpr handle_type create() {
			auto& factory = reg->template get_resource<factory_type>();

			handle_type hnd;

			if (factory.empty())
			{
				hnd = handle_type{ };
				factory.push_back(hnd);
			} 
			else if (handle_type next = factory.back(); next != tombstone{ })
			{
				hnd = handle_type{ factory.size(), { } };
				factory.push_back(hnd);
			}
			else if (value_view{ next } == 0ull)
			{
				hnd = { factory.size() - 1, ++version_view{ factory.back() } };
				factory.back() = hnd;
			}
			else
			{
				
			}

			if constexpr (!std::is_void_v<create_event>) {
				reg->template on<create_event>().invoke(hnd);	// invoke create event
			}
			
			return hnd;
		}
		
		constexpr void destroy(handle_type ent) {
			/*
			auto& factory = reg->template get_resource<factory_type>();


			if (factory.empty()) { 																			// if no active handles
				return;
			} else if (integral_type ind = value_view{ ent }; value_view{ factory[ind - 1] } != ind) {			// if handle already inactive
				return;
			} else if (integral_type next = value_view{ factory.back() }; next == factory.size()) { 		// if no head to inactive list
				emplace_back(handle_type{ ind, -1 });
			} else { 																						// if multiple inactive handles
				factory[ind] = handle_type{ value_view{ factory.back() }, factory[ind] };
				factory.back() = handle_type{ ind, factory.back() };
			}



			
			using entity_component_set = util::filter_t<traits::registry::get_attrib_component_set_t<reg_T>, 
				util::cmp::to_<entity_type, std::is_same, traits::component::get_entity>::template type>;
			util::apply<entity_component_set>([&]<typename ... Ts>() { (reg->template pool<Ts>().erase(ent), ...); });
			
			*/
		}

		constexpr bool alive(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();
			return hnd == fact.active[value_view{ hnd }];
		}
	
		constexpr void clear() {
			/*
			auto& fact = reg->template get_resource<factory_type>();
			
			if constexpr (traits::is_accessible_v<reg_T, event::destroy<entity_type>>)
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
			*/
		}
	private:
		reg_T* reg;
	};
}