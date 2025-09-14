#pragma once
#include "core/traits.h"
#include "containers/sparse_list.h"

namespace ecs {
	template<traits::entity_class T>
	struct factory {
		using ecs_category = tag::attribute;
	private:
		using handle_type = traits::entity::get_handle_t<T>;
		using integral_type = traits::handle::get_attrib_integral_t<handle_type>;
		static constexpr std::size_t version_width = traits::handle::get_attrib_version_width_v<handle_type>;
		
	private:
		// creator function
    };

	template<traits::entity_class T>
	struct archive {
		using ecs_category = tag::attribute;
	private:
		using handle_type = traits::entity::get_handle_t<T>;
		using integral_type = traits::handle::get_attrib_integral_t<handle_type>;
		static constexpr std::size_t version_width = traits::handle::get_attrib_version_width_v<handle_type>;
		
	private:
		// list of old entities
    };

	template<ecs::traits::entity_class T, typename reg_T>
	class generator {
	public:
		using registry_type = reg_T;
		using entity_type = T;
		using handle_type = traits::entity::get_handle_t<T>;
		using integral_type = traits::entity::get_integral_t<T>;
		using factory_type =  traits::entity::get_factory_t<T>;
		using create_event =  traits::entity::get_create_event_t<T>;
		using destroy_event = traits::entity::get_destroy_event_t<T>;
		
		static constexpr bool create_event_enabled = !std::is_void_v<create_event>;
		static constexpr bool destroy_event_enabled = !std::is_void_v<destroy_event>;
		static constexpr bool factory_enabled = !std::is_void_v<factory_type>;
		

		inline constexpr generator(reg_T& reg) noexcept : reg(reg) { }
		
		constexpr handle_type create() {
			handle_type hnd;
			/*
			if constexpr (factory_enabled) {
				auto& ossuary = reg.template get_attribute<ossuary_type>();
				auto& factory = reg.template get_attribute<factory_type>();
				
				hnd = factory.resurrect(ossuary.back());
				ossuary.pop_back();
			} else if constexpr (factory_enabled) {
				auto& factory = reg.template get_attribute<factory_type>();
				
				hnd = factory.create();
			} else if constexpr (ossuary_enabled) {
				auto& ossuary = reg.template get_attribute<ossuary_type>();
				
				hnd = ossuary.back();
				ossuary.pop_back();
			}
			
			if constexpr (create_event_enabled) {
				reg.template on<create_event>().invoke(hnd);
			}
			*/
			return hnd;
		}
		
		constexpr void destroy(handle_type hnd) {
			/*
			if constexpr (ossuary_enabled) {
				auto& ossuary = reg.template get_attribute<ossuary_type>();
				ossuary.push_back(hnd);
			}
			
			if constexpr (factory_enabled) {
				auto& factory = reg.template get_attribute<factory_type>();
				factory.destroy(hnd);
			}

			if constexpr (destroy_event_enabled) {
				reg.template on<destroy_event>().invoke(hnd);
			}
			*/
		}

		constexpr bool alive(handle_type hnd) {
			/*
			if constexpr (ossuary_enabled) {
				if (reg.template get_attribute<ossuary_type>().contains(hnd)) return false;
			}

			if constexpr (factory_enabled) {
				if (!reg.template get_attribute<factory_type>().alive(hnd)) return false;
			}
			*/
			return true;
		}
	
		constexpr void clear() {
			/*
			auto& fact = reg.template get_attribute<factory_type>();
			
			if constexpr (traits::is_accessible_v<reg_T, event::destroy<entity_type>>)
			{
				for (std::size_t i = 0; i < fact.active.size(); ++i)
				{
					handle_type hnd = fact.active[i];
					if (static_cast<integral_type>(value_view{ hnd }) == i) {
						reg.template on<event::destroy<entity_type>>().invoke(hnd);
					}
				}
			}

			fact.active.clear();
			fact.inactive = tombstone{ };
			*/
		}
	private:
		reg_T& reg;
	};
}
