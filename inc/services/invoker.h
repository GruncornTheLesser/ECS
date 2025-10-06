#pragma once
#include "core/traits.h"
#include <functional>

// events
namespace ecs {
	template<traits::event_class T>
	struct listener {
		using ecs_category = ecs::tag::monolith;
		using callback_type = traits::event::get_callback_t<T>;
		using handle_type = std::add_pointer_t<callback_type>;
		using value_type = void;
		using initialize_event = void;
		using terminate_event = void;
	};

	template<ecs::traits::event_class T, typename reg_T>
	struct invoker {
	private:
	template<typename ... Ts> friend class registry;

		using listener_type = traits::event::get_listener_t<T>;
		using sequence_policy = traits::event::get_sequence_policy_t<T>;
		using value_type = traits::component::get_handle_t<listener_type>;
		
		//static constexpr bool fire_once_enabled = traits::event::get_fire_once_v<T>;
	
	
		invoker(reg_T& reg) : reg(reg) { }
	public:

		// constexpr void operator+=(value_type callback) requires(fire_once_enabled) { attach(callback, false); }
		constexpr void operator+=(value_type callback) /*requires(!fire_once_enabled)*/ { attach(callback); }
		// constexpr void operator^=(value_type callback) requires(fire_once_enabled) { attach(callback, true); }
		//constexpr void operator^=(value_type callback) /*requires(!fire_once_enabled)*/ { attach(callback); }
		
		constexpr void operator-=(value_type handle) { detach(handle); }
	
		template<typename ... arg_Ts> constexpr void invoke(arg_Ts&& ... args) {
			for (auto [listener] : reg.template view<ecs::entity>(ecs::from<listener_type>{})) {
				std::invoke(listener, std::forward<arg_Ts>(args)...);
			}
		}

		//constexpr void attach(value_type callback, bool fire_once) requires(fire_once_enabled) {
		//	reg.template emplace<listener_type>(callback);
		//}
		
		constexpr void attach(value_type callback) /*requires(!fire_once_enabled)*/ {
			reg.template emplace<listener_type>(callback);
		}
		

		template<typename seq_T=sequence_policy>
		constexpr void detach(value_type callback) { 
			reg.template erase<listener_type, seq_T>(callback);
		}
	
		constexpr void clear() {
			reg.template pool<listener_type>().clear();

		}
	private:
		reg_T& reg;
	};
}