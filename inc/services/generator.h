#pragma once
#include "core/traits.h"

namespace ecs {
	template<traits::entity_class T, typename reg_T>
	class generator {
		using registry_type = reg_T;
		using entity_type = T;
		using handle_type = traits::entity::get_handle_t<T>;
		using factory_type = traits::entity::get_factory_t<T>;
		using create_event = traits::entity::get_create_event_t<T>;
		using destroy_event = traits::entity::get_destroy_event_t<T>;
		
		static constexpr bool create_event_enabled = !std::is_void_v<create_event>;
		static constexpr bool destroy_event_enabled = !std::is_void_v<destroy_event>;
		
		static_assert(!std::is_void_v<factory_type>);
		
	public:
		inline constexpr generator(reg_T& reg) noexcept : reg(reg) { }
		
		template<typename ... arg_Ts>
		constexpr inline handle_type create(arg_Ts&& ... args) {
			auto& factory = reg.template get_attribute<factory_type>();
			handle_type hnd = factory.create(std::forward<arg_Ts>(args)...);

			if constexpr (create_event_enabled) {
				reg.template on<create_event>().invoke(hnd);
			}

			return hnd;
		}
		
		constexpr inline void destroy(handle_type hnd) {
			if (!alive(hnd)) return;

			if constexpr (destroy_event_enabled) {
				reg.template on<destroy_event>().invoke(hnd);
			}

			auto& factory = reg.template get_attribute<factory_type>();
			factory.destroy(hnd);
		}

		constexpr inline bool alive(handle_type hnd) {
			return reg.template get_attribute<factory_type>().alive(hnd);
		}
	
		constexpr inline void clear() {
			// tricky...
		}
	private:
		reg_T& reg;
	};
}
