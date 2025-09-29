#pragma once
#include "core/traits.h"
#include <functional>

// events
namespace ecs {
	template<traits::event_class T>
	struct listener {
		using ecs_category = ecs::tag::resource;
		using handle_type = std::size_t;
		using value_type = std::function<traits::event::get_callback_t<T>>;
		using create_event = void;
		using destroy_event = void;
		using manager_type = void;
	};

	template<ecs::traits::event_class T, typename reg_T>
	struct invoker {
	private:
		using listener_type = traits::event::get_listener_t<T>;
		using callback_type = traits::event::get_callback_t<T>;
		using handle_type = traits::component::get_handle_t<listener_type>;
	
		static constexpr bool fire_once_enabled = false;
	public:
		invoker(reg_T& reg) : reg(reg) { }

		constexpr handle_type operator+=(callback_type&& callback) { return attach(std::forward<callback_type>(callback), false); }
		constexpr handle_type operator^=(callback_type&& callback) { return attach(std::forward<callback_type>(callback), true); }
		
		constexpr bool operator-=(handle_type handle) { return detach(handle); }
	
	

		template<typename ... arg_Ts> constexpr void invoke(arg_Ts&& ... args) {
			for (auto [listener] : reg.template rview<listener_type>()) {
				std::invoke(listener, std::forward<arg_Ts>(args)...);

			}
		}

		constexpr handle_type attach(callback_type&& callback, bool once=false) {
			// auto ent = reg.template generator<entity_type>().create();
			// reg.template emplace<listener_type>(ent, std::forward<callback_type>(callback));
			// if constexpr (fire_once_enabled) {
			// 	if (once) {
			// 		reg.template emplace<once_flag>(ent);
			// 	}
			// }
			// return ent;
		}

		template<typename ... arg_Ts> 
		constexpr handle_type attach(callback_type&& callback, bool once=false, arg_Ts&& ... args) {
			// auto ent = reg.template generator<entity_type>().create();
			// reg.template emplace<listener_type>(ent, std::bind(std::forward<callback_type>(callback), std::placeholders::_1, std::forward<arg_Ts>(args)...));
			// if constexpr (fire_once_enabled) {
			// 	if (once) {
			// 		reg.template emplace<once_flag>(ent);
			// 	}
			// }
			// return ent;
		}

		
		template<typename seq_T=policy::strict>
		constexpr void detach(handle_type ent) { 
			// reg.template erase<seq_T>(ent);
			// reg.template generator().destroy(ent);
		}
	
		constexpr void clear() {
			// reg.template generator<entity_type>().clear();
		}
	private:
		reg_T& reg;
	};
}