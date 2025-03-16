#pragma once
#include <utility>
#include <type_traits>

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
		struct element;

		template<std::size_t I, typename T>
		struct element_element;
	}

	template<typename ... Ts>
	struct element : private details::element<std::index_sequence_for<Ts...>, Ts...> {
		
		inline constexpr element() = default;
		inline constexpr element(const element&) = default;
		inline constexpr element& operator=(const element&) = default;
		
		inline constexpr element(element&&) = default;
		inline constexpr element& operator=(element&&) = default;
		
		template<typename ... Us> inline constexpr element(Us&& ... args)
		 : details::element<std::index_sequence_for<Ts...>, Ts...>(std::forward<Ts>(args)...) { }
		
		inline constexpr operator std::tuple_element_t<0, ecs::element<Ts...>>() {
			return details::element_element<0, std::tuple_element_t<0, ecs::element<Ts...>>>::value;
		}

		template<std::size_t I>
		inline constexpr std::tuple_element_t<I, ecs::element<Ts...>> get() {
			return details::element_element<I, std::tuple_element_t<I, ecs::element<Ts...>>>::value;
		}
	};

	namespace details {
		template<std::size_t I, typename T>
		struct element_element {
			template<typename...> friend struct ecs::element;

			inline constexpr element_element() = default;
			inline constexpr element_element(T&& value) : value(value) { }
			
			inline constexpr element_element(const element_element&) = default;
			inline constexpr element_element& operator=(const element_element&) = default;
			
			inline constexpr element_element(element_element&&) = default;
			inline constexpr element_element& operator=(element_element&&) = default;
		private:
			T value;
		};
	
		template<std::size_t ... Is, typename ... Ts>
		struct element<std::index_sequence<Is...>, Ts...> : private element_element<Is, Ts>... {
			template<typename...> friend struct ecs::element;

			inline constexpr element() = default;
			template<typename ... Us> inline constexpr element(Us&& ... val) : element_element<Is, Ts>(std::forward<Ts>(val))... { }
			
			inline constexpr element(const element&) = default;
			inline constexpr element& operator=(const element&) = default;
			
			inline constexpr element(element&&) = default;
			inline constexpr element& operator=(element&&) = default;
		};
	}
}

namespace std {
	template<typename ... Ts> 
	class tuple_size<ecs::element<Ts...>>
	 : std::integral_constant<std::size_t, sizeof...(Ts)> { };
	
	template<std::size_t N, typename T, typename ... Ts>
	class tuple_element<N, ecs::element<T, Ts...>> : public tuple_element<N - 1, ecs::element<Ts...>> { };

	template<typename T, typename ... Ts>
	class tuple_element<0, ecs::element<T, Ts...>> : public std::type_identity<T> { };
}
