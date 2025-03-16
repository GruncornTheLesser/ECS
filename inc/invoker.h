#pragma once
#include <functional>
#include <tuple>
#include "traits.h"

// events
namespace ecs::event {
	template<traits::resource_class T> struct acquire {
		using ecs_tag = ecs::tag::event_sync; // cannot be async
		T& value;
	};

	template<traits::resource_class T> struct release {
		using ecs_tag = ecs::tag::event_sync; // cannot be async
		T& value;
	};

	template<traits::table_class T> struct create {
		using ecs_tag = ecs::tag::event;
		typename table_traits<T>::handle_type handle;
	};

	template<traits::table_class T> struct destroy {
		using ecs_tag = ecs::tag::event;
		typename table_traits<T>::handle_type handle;
	};
	
	template<traits::component_class T> struct init {
		using ecs_tag = ecs::tag::event;

		typename table_traits<typename component_traits<T>::table_type>::handle_type handle;
		typename component_traits<T>::storage_type::value_type& value;
	};
	
	template<traits::component_class T> struct terminate {
		using ecs_tag = ecs::tag::event;

		typename table_traits<typename component_traits<T>::table_type>::handle_type handle;
		typename component_traits<T>::storage_type::value_type& value;
	};
}

namespace ecs {
	template<typename T>
	struct invoker {
	private:
		using handle_type = event_traits<T>::handle_type; // TODO: use handle type and implement versioning
		using listener_type = typename event_traits<T>::listener_type;
		static constexpr bool enable_async = event_traits<T>::enable_async;
		static constexpr bool strict_order = event_traits<T>::strict_order;
		struct element_type { uint32_t handle; listener_type listener = nullptr; };
	public:
		using ecs_tag = tag::resource;
		
		inline constexpr void operator()(const T& event) { invoke(event); }
		inline constexpr handle_type operator^=(listener_type listener) { return listen(std::forward<listener_type>(listener), true); }
		inline constexpr handle_type operator+=(listener_type listener) { return listen(std::forward<listener_type>(listener), false); }
		inline constexpr bool operator-=(handle_type handle) { return detach(handle); }
	
		constexpr void invoke(T event) {
			std::size_t i = 0;
			while (i < recurring)
			{
				// recurring events
				// TODO: async events
				//if constexpr (enable_async)
				//	data[i].listener(event); 
				//else 
				data[i].listener(event);
				++i;
			}
			while (i < fire_once)
			{
				// TODO: async events
				//if constexpr (enable_async)
				//	data[i].listener(event); 
				//else 
				data[i].listener(event); // fire_once listeners
				data[i].listener = nullptr;
				++i;
			}
			fire_once = recurring; // clear fire once sections
		}

		constexpr handle_type listen(listener_type listener, bool once) {
			uint32_t handle;
			
			if (fire_once == data.size()) { // storage is full
				// allocate and generate new handle
				handle = data.size();
				data.resize(data.size() + 1);
			}
			else {
				handle = data[fire_once].handle; // reuse handle
			}

			if (once) {
				// add listener to back of fire once partition
				data[fire_once] = { handle, std::forward<listener_type>(listener) };
				
				// increment offsets
				++fire_once;
				
				return handle;
			}
			else {
				// make space for new listener
				if constexpr (strict_order)
					std::move_backward(data.begin() + recurring, data.begin() + fire_once, data.begin() + fire_once + 1);
				else
					data[fire_once] = std::move(data[recurring]);
				
				// add listener to back of recurring section
				data[recurring] = { handle, std::forward<listener_type>(listener) };
				
				// increment offsets
				++fire_once;
				++recurring;

				return handle;
			}
		}
	
		constexpr bool detach(handle_type handle) {
			// find index
			int i = 0;
			while (i < fire_once && data[i].handle != handle);
			
			if (i == fire_once) return false; // if i == end of active partition
			
			if constexpr (strict_order)
			{
				// move active after i back 1
				std::move(data.begin() + i + 1, data.begin() + fire_once, data.begin() + i);
				if (i < recurring) --recurring;
				--fire_once;
			}
			else
			{
				if (i < recurring) {
					data[i] = std::move(data[--recurring]);
					i = recurring;
				}
				data[i] = std::move(data[--fire_once]);
			}
			data[fire_once].handle = handle;
			return true;
		}
	
		constexpr void clear() {
			fire_once = 0;
			recurring = 0;
			data.clear();
		}

	private:
		std::size_t recurring = 0, fire_once = 0;
		std::vector<element_type> data;
	};
}

namespace ecs::test {
	template<typename T, typename reg_T>
	struct invoker {
		using handle_type = event_traits<T>::handle_type;
		using storage_type = int;
		using manager_type = int;
		using listener_type = typename event_traits<T>::listener_type;
		static constexpr bool enable_async = event_traits<T>::enable_async;
		static constexpr bool strict_order = event_traits<T>::strict_order;
	private:
		struct element_type { uint32_t handle; listener_type listener = nullptr; };
	public:
		inline constexpr invoker(reg_T reg) : reg(reg) { }
		
		inline constexpr void operator()(const T& event) { invoke(event); }
		inline constexpr bool operator-=(handle_type handle) { return detach(handle); }
		inline constexpr handle_type operator^=(listener_type listener) { return listen(std::forward<listener_type>(listener), true); }
		inline constexpr handle_type operator+=(listener_type listener) { return listen(std::forward<listener_type>(listener), false); }

		handle_type listen(listener_type listener, bool once) {
			// reg->template get_resource<storage_type>();
			// reg->template get_resource<manager_type>();
		}

	private:
		reg_T* reg;
	};





}