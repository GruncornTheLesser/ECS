#pragma once 
#include "core/fwd.h"
#include <vector>
#include <vector>
#include <unordered_map>
namespace ecs {
    template<typename T>
	struct factory {
		template<typename,typename> friend class generator;
		
		using ecs_tag = ecs::tag::resource;
		using entity_type = T;
		using handle_type = typename entity_traits<T>::handle_type;
		using value_type = typename handle_traits<handle_type>::value_type;
		using version_type = typename handle_traits<handle_type>::version_type;
		using integral_type = typename handle_traits<handle_type>::integral_type;

	private:
		value_type inactive{ tombstone{ } };
		std::vector<handle_type> active;
	};

    template<typename T> struct manager
        : std::vector<typename entity_traits<typename component_traits<T>::entity_type>::handle_type> { 
        using ecs_tag = ecs::tag::resource;
    };
    template<typename T> struct indexer : std::unordered_map<
        typename handle_traits<typename entity_traits<typename component_traits<T>::entity_type>::handle_type>::integral_type,
        typename entity_traits<typename component_traits<T>::entity_type>::handle_type> {
        using ecs_tag = ecs::tag::resource;
    };
    template<typename T> struct storage : std::vector<T> { 
        using ecs_tag = ecs::tag::resource;
    };
}
