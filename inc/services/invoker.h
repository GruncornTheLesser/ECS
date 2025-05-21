#pragma once
#include "core/traits.h"

// events
namespace ecs {
	template<ecs::traits::event_class T, typename reg_T>
	struct invoker {
	public:
		using listener_type = traits::event::get_listener_t<T>;
		using callback_type = traits::event::get_callback_t<T>;
		using entity_type = traits::component::get_entity_t<listener_type>;
		using handle_type = traits::entity::get_handle_t<entity_type>;

		static constexpr bool enable_async = traits::event::get_enable_async_v<T>; // TODO: asynchronous events, currently disabled
		static constexpr bool strict_order = traits::event::get_strict_order_v<T>;  // TODO: strict order events, currently disabled
		
		static constexpr bool enable_fire_once = false;
	public:
		using ecs_tag = tag::resource;
		
		invoker(reg_T* reg) : reg(reg) { }

		[[nodiscard]] inline constexpr handle_type operator+=(callback_type&& callback) { return attach(std::forward<callback_type>(callback)); }
		inline constexpr bool operator-=(handle_type handle) { return detach(handle); }
	
		template<typename ... arg_Ts> constexpr void invoke(arg_Ts&& ... args) {
			
			for (auto [listener] : reg->template view<listener_type>()) {
				listener.operator()(std::forward<arg_Ts>(args)...);
			}
		}

		[[nodiscard]] constexpr handle_type attach(callback_type&& callback) {
			auto ent = reg->template generator<entity_type>().create();
			reg->template pool<listener_type>().emplace(ent, std::forward<callback_type>(callback));
			return ent;
		}
	
		constexpr void detach(handle_type ent) { 
			reg->template pool<listener_type>().erase(ent);
			reg->template generator<entity_type>().destroy(ent);
		}
	
		constexpr void clear() {
			reg->template generator<T>().clear();
		}
	private:
		reg_T* reg;
	};
}