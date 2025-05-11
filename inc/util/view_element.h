#pragma once
#include "core/meta.h"

/*
Notes: expression types constness decays on assignment. 
In structured binding that means:
auto [a1, b1] = tuple<A, B>{ _a, _b };
auto [a2, b2] = std::as_const(tuple<A, B>{ _a, _b });
static_assert(std::is_same_v<decltype(a1), decltype(a2)>);
static_assert(std::is_same_v<decltype(b1), decltype(b2)>);

*/

namespace ecs {
	namespace details {
		template<typename Ind, typename ... Ts>
		struct view_element;

		template<std::size_t I, typename T>
		struct view_element_index;
	}

	template<typename ... Ts>
	struct view_element : private details::view_element<std::index_sequence_for<Ts...>, Ts...> {
		
		inline constexpr view_element() = default;
		inline constexpr view_element(const view_element&) = default;
		inline constexpr view_element& operator=(const view_element&) = default;
		
		inline constexpr view_element(view_element&&) = default;
		inline constexpr view_element& operator=(view_element&&) = default;
		
		template<typename ... Us> inline constexpr view_element(Us&& ... args)
		 : details::view_element<std::index_sequence_for<Ts...>, Ts...>(std::forward<Ts>(args)...) { }
		
		inline constexpr operator meta::arg_element_t<0, ecs::view_element<Ts...>>() {
			return details::view_element_index<0, meta::arg_element_t<0, ecs::view_element<Ts...>>>::value;
		}

		template<std::size_t I>
		inline constexpr meta::arg_element_t<I, ecs::view_element<Ts...>> get() {
			return details::view_element_index<I, meta::arg_element_t<I, ecs::view_element<Ts...>>>::value;
		}
	};

	namespace details {
		template<std::size_t I, typename T>
		struct view_element_index {
			template<typename...> friend struct ecs::view_element;

			inline constexpr view_element_index() = default;
			inline constexpr view_element_index(T&& value) : value(value) { }
			
			inline constexpr view_element_index(const view_element_index&) = default;
			inline constexpr view_element_index& operator=(const view_element_index&) = default;
			
			inline constexpr view_element_index(view_element_index&&) = default;
			inline constexpr view_element_index& operator=(view_element_index&&) = default;
		private:
			T value;
		};
	
		template<std::size_t ... Is, typename ... Ts>
		struct view_element<std::index_sequence<Is...>, Ts...> : private view_element_index<Is, Ts>... {
			template<typename...> friend struct ecs::view_element;

			inline constexpr view_element() = default;
			template<typename ... Us> inline constexpr view_element(Us&& ... val) : view_element_index<Is, Ts>(std::forward<Ts>(val))... { }
			
			inline constexpr view_element(const view_element&) = default;
			inline constexpr view_element& operator=(const view_element&) = default;
			
			inline constexpr view_element(view_element&&) = default;
			inline constexpr view_element& operator=(view_element&&) = default;
		};
	}
}

namespace std {
	template<typename ... Ts> 
	class tuple_size<ecs::view_element<Ts...>> : public std::integral_constant<std::size_t, sizeof...(Ts)> { };
	
	template<std::size_t N, typename T, typename ... Ts>
	class tuple_element<N, ecs::view_element<T, Ts...>> : public tuple_element<N - 1, ecs::view_element<Ts...>> { };

	template<typename T, typename ... Ts>
	class tuple_element<0, ecs::view_element<T, Ts...>> : public std::type_identity<T> { };
}
