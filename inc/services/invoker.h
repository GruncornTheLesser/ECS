#pragma once
#include "core/traits.h"
#include <functional>

// events
namespace ecs {
	template<ecs::traits::event_class T, typename reg_T>
	struct invoker {
	public:
		using listener_type = traits::event::get_listener_t<T>;
		using callback_type = traits::event::get_callback_t<T>;
		using entity_type = traits::component::get_entity_t<listener_type>;
		using handle_type = traits::entity::get_handle_t<entity_type>;

	public:
		invoker(reg_T& reg) : reg(reg) { }

		inline constexpr handle_type operator+=(callback_type&& callback) { return attach(std::forward<callback_type>(callback), false); }
		inline constexpr handle_type operator^=(callback_type&& callback) { return attach(std::forward<callback_type>(callback), true); }
		
		inline constexpr bool operator-=(handle_type handle) { return detach(handle); }
	
	

		template<typename ... arg_Ts> constexpr void invoke(arg_Ts&& ... args) {
			for (auto [listener] : reg.template view<listener_type>()) {
				listener.operator()(std::forward<arg_Ts>(args)...);
			}
		}

		constexpr handle_type attach(callback_type&& callback, bool once=false) {
			auto ent = reg.template generator<entity_type>().create();
			reg.template emplace<listener_type>(ent, std::forward<callback_type>(callback));
			return ent;
		}

		template<typename ... arg_Ts> 
		constexpr handle_type attach(callback_type&& callback, bool once=false, arg_Ts&& ... args) {
			auto ent = reg.template generator<entity_type>().create();
			reg.template emplace<listener_type>(ent, std::bind(std::forward<callback_type>(callback), std::placeholders::_1, std::forward<arg_Ts>(args)...));
			return ent;
		}

	
		constexpr void detach(handle_type ent) { 
			reg.template pool<listener_type>().erase(ent);
			reg.template generator<entity_type>().destroy(ent);
		}
	
		constexpr void clear() {
			reg.template generator<entity_type>().clear();
		}
	private:
		reg_T& reg;
	};
}