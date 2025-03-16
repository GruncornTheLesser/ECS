#pragma once
#include "traits.h"
#include "handle.h"

namespace ecs {
	template<typename T>
	struct factory {
		template<typename,typename> friend class generator;
		
		using ecs_tag = ecs::tag::resource;
		using table_type = T;
		using handle_type = handle<uint32_t, 12>; // TODO: FIXME
		using value_type = typename handle_traits<handle_type>::value_type;
		using version_type = typename handle_traits<handle_type>::version_type;
		using integral_type = typename handle_traits<handle_type>::integral_type;

	private:
		value_type inactive{ tombstone{ } };
		std::vector<handle_type> active;
	};

	template<typename T, typename reg_T>
	class generator {
	public:
		using registry_type = reg_T;
		using table_type = T;
		using handle_type = typename table_traits<T>::handle_type;
		using factory_type = typename table_traits<T>::factory_type;
		
		using value_type = typename handle_traits<handle_type>::value_type;
		using version_type = typename handle_traits<handle_type>::version_type;
		using integral_type = typename handle_traits<handle_type>::integral_type;

		inline constexpr generator(reg_T* reg) noexcept : reg(reg) { }
		
		constexpr handle_type create() {
			// initialize with new handle active.size() and version 0
			auto& fact = reg->template get_resource<factory_type>();
			
			if (fact.inactive == tombstone{ }) {
				return fact.active.emplace_back(handle_type{ static_cast<integral_type>(fact.active.size()) });
			}

			handle_type tmp = fact.active[fact.inactive]; // get next in inactive list
			handle_type hnd = handle_type{ fact.inactive, version_type{ tmp } }; // resurrect previous handle
			
			fact.active[fact.inactive] = hnd;
			fact.inactive = value_type{ tmp }; // increment inactive list
			
			if constexpr (traits::is_accessible_v<event::create<table_type>, reg_T>) {
				reg->template on<event::create<table_type>>().invoke({ hnd });
			}
			
			return hnd;
		}
		
		constexpr void destroy(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();

			std::size_t pos = value_type{ hnd }; // get handle value
			if (fact.active[pos] != hnd) return; // if not alive
			
			if constexpr (traits::is_accessible_v<event::destroy<table_type>, reg_T>) {
				reg->template on<event::destroy<table_type>>().invoke({hnd});
			}

			fact.active[pos] = handle_type{ fact.inactive, version_type{ hnd } };
			fact.inactive = pos;
		}

		constexpr bool alive(handle_type hnd) {
			auto& fact = reg->template get_resource<factory_type>();
			return hnd == fact.active[value_type{ hnd }];
		}
	
		constexpr void clear() {
			auto& fact = reg->template get_resource<factory_type>();
			
			if constexpr (traits::is_accessible_v<event::destroy<table_type>, reg_T>)
			{
				for (std::size_t i = 0; i < fact.active.size(); ++i)
				{
					handle_type hnd = fact.active[i];
					if (static_cast<integral_type>(value_type{ hnd }) == i) {
						reg->template on<event::destroy<table_type>>().invoke({ hnd });
					}
				}
			}

			fact.active.clear();
			fact.inactive = value_type{ tombstone{ } };
		}
	private:
		reg_T* reg;
	};
}