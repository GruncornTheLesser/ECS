#pragma once
#include "core/fwd.h"

// events
namespace ecs {
	template<typename T, typename reg_T>
	struct invoker {
	public:
		using entity_type = typename event_traits<T>::entity_type;
		using handle_type = typename entity_traits<entity_type>::handle_type;
		using listener_type = typename event_traits<T>::listener_type;
		using callback_type = typename event_traits<T>::callback_type;
		static constexpr bool enable_async = event_traits<T>::enable_async; // TODO: asynchronous events, currently disabled
		static constexpr bool strict_order = event_traits<T>::strict_order;  // TODO: strict order events, currently disabled
		static constexpr bool enable_fire_once = false;
	public:
		using ecs_tag = std::conditional_t<enable_async, tag::resource_unrestricted, tag::resource>;
		
		invoker(reg_T* reg) : reg(reg) { }

		[[nodiscard]] inline constexpr handle_type operator+=(callback_type&& callback) { return attach(std::forward<callback_type>(callback)); }
		inline constexpr bool operator-=(handle_type handle) { return detach(handle); }
	
		template<typename ... arg_Ts> constexpr void invoke(arg_Ts&& ... args) {
					
			for (listener_type& listener : reg->template view<listener_type>()) {
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
			reg->template generator<entity_type>().destroy();
		}
	
		constexpr void clear() {
			reg->template generator<T>().clear();
		}
	private:
		reg_T* reg;
	};
}