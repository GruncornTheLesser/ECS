#pragma once 
#include "core/traits.h"
#include "containers/packed.h"
#include "containers/sparse.h"
#include <unordered_map>
#include <set>

namespace ecs {
	template<traits::component_class T>
	struct manager { 
		using ecs_category = tag::attribute;
		using value_type = ecs::packed<traits::component::get_handle_t<T>>;
		
		static constexpr int init_priority = 1;
		static void destroy(auto& reg, value_type& val) {
			reg.template pool<T>().clear();
		}
	};
	
	template<traits::component_class T>
	struct indexer {
		using ecs_category = tag::attribute;
		using value_type = std::conditional_t<std::is_void_v<traits::component::get_manager_t<T>>, std::set<traits::component::get_handle_t<T>>, std::conditional_t<std::is_assignable_v<std::size_t, traits::component::get_handle_t<T>>, ecs::sparse<std::size_t, traits::component::get_page_size_v<T>>, std::unordered_map<traits::component::get_handle_t<T>, std::size_t>>>;
	};

	template<traits::component_class T>
	struct storage {
		using ecs_category = tag::attribute;
		using value_type = ecs::packed<traits::component::get_value_t<T>>;
	};

	template<traits::entity_class T>
	struct factory {
		using ecs_category = tag::attribute;
		using handle_type = traits::entity::get_handle_t<T>;

		using destroy_event = traits::entity::get_destroy_event_t<T>;

		static constexpr bool destroy_event_enabled = !std::is_void_v<destroy_event>;


		static constexpr int init_priority = -255;
		
		
		static void destroy(auto& reg, factory<T>& val) {
			if constexpr (destroy_event_enabled) {
				for (std::size_t ind = 0; ind < val.version.size(); ++ind) {
					if (std::size_t{ val.version.at(ind) } == ind) {
						reg.template on<destroy_event>().invoke(reg, val.version.at(ind));
					}
				}
			}
		}

		constexpr handle_type create() {
			if (next == tombstone{}) {
				return version.emplace_back(version.size());
			} else {
				std::size_t ind = next;
				auto& curr = ++version.at(ind);
				next = curr;
				curr = handle_type{ ind, curr };
				return curr;
			}
		}

		constexpr void destroy(handle_type hnd) { 
			if (!alive(hnd)) {
				return;
			}

			auto& curr = version.at(hnd);
			curr = handle_type{ next, curr };
			next = hnd;

			assert(next == tombstone{} || std::size_t{ next } < version.size());
		}

		constexpr bool alive(handle_type hnd) const { 
			std::size_t ind = hnd;
			return (ind < version.size() && version.at(ind) == hnd);
		}

	private:
		handle_type next = tombstone{};
		ecs::packed<traits::entity::get_handle_t<T>> version;
	};
}
