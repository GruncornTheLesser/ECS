#pragma once
#include "core/traits.h"

#if ECS_DYNAMIC_REGISTRY
#include "content/id.h"
#include <unordered_map>
#endif

#include <memory>
#include <type_traits>

namespace ecs {
	template<typename ... Ts> 
	class registry {
	public:
		struct erased_cache_t {
			virtual void construct(registry<Ts...>& reg) = 0;
			virtual void destroy(registry<Ts...>& reg) = 0;
			virtual void acquire(registry<Ts...>& reg, priority p) = 0;
			virtual void acquire(registry<Ts...>& reg, priority p) const = 0;
			virtual void release(registry<Ts...>& reg) = 0;
			virtual void release(registry<Ts...>& reg) const = 0;
		};
		
		template<traits::attribute_class T>
		struct cache_t final : erased_cache_t  { 
			using value_type = traits::attribute::get_value_t<T>;
			using mutex_type = util::eval_if_t<traits::attribute::get_mutex_t<T>, std::is_void, 
				util::wrap_<std::type_identity>::template type>; // std::type_identity is empty
			
			using acquire_event = traits::attribute::get_acquire_event<T>;
			using release_event = traits::attribute::get_release_event<T>;
			
			static constexpr bool acquire_enabled = !std::is_void_v<acquire_event>;
			static constexpr bool release_enabled = !std::is_void_v<release_event>;

			cache_t() { }
			~cache_t() { }
			cache_t(const cache_t&) = delete;
			cache_t& operator=(const cache_t&) = delete;
			cache_t(cache_t&& other) { 
				std::construct_at(&value, std::move(other.value));
				std::construct_at(&mutex, std::move(other.mutex));
			}
			cache_t& operator=(cache_t&& other) {
				if (this == &other) return *this;
				value = std::move(other.value);
				mutex = std::move(other.mutex);
				return *this;
			}

			void construct(registry<Ts...>& reg) override {
				std::construct_at(&mutex);

				if constexpr (requires { T::construct(&value, reg); }) {
					T::construct(&value, reg);
				} else {
					std::construct_at(&value);
				}
			}
				
			void destroy(registry<Ts...>& reg) override {	
				if constexpr (requires { T::destroy(&value, reg); }) {
					T::destroy(&value, reg); 
				} else {
					std::destroy_at(&value);
				}
				
				std::destroy_at(&mutex);
			}
	
			void acquire(registry<Ts...>& reg, priority p) override { 
				if constexpr (requires { mutex.lock(p); }) {
					mutex.lock(p); 
				}
				else if constexpr (requires { mutex.lock(); }) {
					mutex.lock(); 
				}
				
				if constexpr (acquire_enabled) {
					reg.template on<acquire_event>().invoke(value);
				}
			}
	
			void release(registry<Ts...>& reg) override { 
				if constexpr (requires { mutex.unlock(); }) {
					mutex.unlock(); 
				}

				if constexpr (release_enabled) {
					reg.template on<release_event>().invoke(value);
				}
			}
	
			void acquire(registry<Ts...>& reg, priority p) const override { 
				if constexpr (requires { mutex.shared_lock(p); }) {
					mutex.shared_lock(); 
				}
				else if constexpr (requires { mutex.shared_lock(); }) {
					mutex.shared_lock(); 
				}
				else if constexpr (requires { mutex.lock(p); }) {
					mutex.lock(p); 
				}
				else if constexpr (requires { mutex.lock(); }) {
					mutex.lock(); 
				}

				if constexpr (acquire_enabled) {
					reg.template on<acquire_event>().invoke(value);
				}
			}
	
			void release(registry<Ts...>& reg) const override {
				if constexpr (requires { mutex.shared_lock(); }) {
					mutex.shared_unlock(); 
				}
				else if constexpr (requires { mutex.lock(); }) {
					mutex.unlock(); 
				}

				if constexpr (release_enabled) {
					reg.template on<release_event>().invoke(value);
				}
			}
	
			union { [[no_unique_address]] mutable mutex_type mutex; };
			union { [[no_unique_address]] value_type value; };
		};
	public:
		using static_dependencies = traits::dependencies::get_attribute_set_t<Ts...>;
		using static_cache_set_t = util::eval_each_t<static_dependencies, util::wrap_<cache_t>::template type>;
		
		#if ECS_DYNAMIC_REGISTRY
		using dynamic_cache_set_t = std::unordered_map<ecs::id, erased_cache_t*>;
		#endif
	
	public:
		registry() {
			using init_sequence = util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>;
			util::apply<init_sequence>([&]<typename ... attrib_Ts>() { 
				(get_cache<attrib_Ts>().construct(*this), ...);
			});
		};
		~registry() {
			using term_sequence = util::reverse_t<util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>>;
			util::apply<term_sequence>([&]<typename ... attrib_Ts>() { 
				(get_cache<attrib_Ts>().destroy(*this), ...);
			});
			
			#if ECS_DYNAMIC_REGISTRY
			for (auto& [id, cache] : dynamic_cache_set) { 
				cache->destroy(*this);
				delete cache;
			}
			#endif
		}
		registry(const registry&) = delete;
		registry& operator=(const registry&) = delete;
		registry(registry&& other) { 
			if (this == &other) return;
			
			using move_sequence = util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>;
			util::apply<move_sequence>([&]<typename ... attrib_Ts>() { 
				(std::move(get_cache<attrib_Ts>(), other.get_cache<attrib_Ts>()), ...);
			});

			#if ECS_DYNAMIC_REGISTRY
			dynamic_cache_set = std::move(other.dynamic_cache_set);
			#endif
		}
		registry& operator=(registry&& other) { 
			if (this == &other) return *this;

			using move_sequence = util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>;
			util::apply<move_sequence>([&]<typename ... attrib_Ts>() { 
				(std::move(get_cache<attrib_Ts>(), other.get_cache<attrib_Ts>()), ...);
			});

			#if ECS_DYNAMIC_REGISTRY
			for (auto& [id, cache] : dynamic_cache_set) { 
				cache->destroy(*this);
				delete cache;
			}
			dynamic_cache_set = std::move(other.dynamic_cache_set);
			#endif

			return *this;
		}

		#if ECS_DYNAMIC_REGISTRY
		template<typename ... Us> requires (!traits::is_attribute_v<Ts> && ...)
		inline void cache() {
			using cache_sequence = util::eval_t<
				traits::dependencies::get_attribute_set_t<Ts...>, 
				util::sort_by_<traits::attribute::get_init_priority>::template type,
				util::filter_<util::pred::element_of_<static_dependencies>::template type>::template type
			>;
			
			util::apply_each<cache_sequence>([&]<typename T>{ 
				if (auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.end()) {
					cache_t<T>* cache = new cache_t<T>(); // allocate cache
					cache->construct(*this);
	
					it = dynamic_cache_set.emplace_hint(it, ecs::id(std::type_identity<T>{}), cache);
				}
			});
		}
		#endif

		/* caches a attribute, creates a new attribute cache if one isnt already cached. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>) 
		inline cache_t<T>& cache() {
			return std::get<cache_t<T>>(static_cache_set);
		}
		
		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>) 
		inline cache_t<T>& cache() {
			if (auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.end()) {
				cache_t<T>* cache = new cache_t<T>(); // allocate cache
				cache->construct(*this);

				it = dynamic_cache_set.emplace_hint(it, ecs::id(std::type_identity<T>{}), cache);
			}
			else {
				return *static_cast<cache_t<T>*>(it->second);
			}
		}
		#endif
		
		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<cache_t<std::remove_const_t<T>>, T>& get_cache() {
			return std::get<cache_t<std::remove_const_t<T>>>(static_cache_set);
		}

		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<cache_t<std::remove_const_t<T>>>& get_cache() const {
			return std::get<cache_t<std::remove_const_t<T>>>(static_cache_set);
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<cache_t<std::remove_const_t<T>>, T>&  get_cache() {
			return *static_cast<const cache_t<T>*>(dynamic_cache_set.at(std::type_identity<T>{}));
		}
		
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<cache_t<std::remove_const_t<T>>>& get_cache() const {
			return *static_cast<const cache_t<T>*>(dynamic_cache_set.at(std::type_identity<T>{}));
		}
		#endif

		/* returns a pointer to a attribute type if cached. if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<cache_t<std::remove_const_t<T>>, T>* try_cache() {
			return &std::get<cache_t<std::remove_const_t<T>>>(static_cache_set);
		}

		/* returns a pointer to a attribute type if cached. if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<cache_t<std::remove_const_t<T>>>* try_cache() const {
			return &std::get<cache_t<std::remove_const_t<T>>>(static_cache_set);
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<cache_t<std::remove_const_t<T>>, T>* try_cache() {
			if(auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.cend()) {
				return nullptr;
			} 
			else {
				return static_cast<cache_t<std::remove_const_t<T>>*>(it->second);
			}
		}
		
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<cache_t<std::remove_const_t<T>>>* try_cache() const {
			if(auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.cend()) {
				return nullptr;
			} else {
				return static_cast<const cache_t<std::remove_const_t<T>>*>(it->second);
			}
		}
		#endif

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T>
		inline util::copy_const_t<traits::attribute::get_value_t<T>, T>& get_attribute() {
			return get_cache<T>().value;
		}

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T>
		inline std::add_const_t<traits::attribute::get_value_t<T>>& get_attribute() const {
			return get_cache<T>().value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<traits::attribute::get_value_t<T>, T>* try_attribute() {
			return &std::get<cache_t<std::remove_const_t<T>>*>(static_cache_set).value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<traits::attribute::get_value_t<T>>* try_attribute() const {
			return &std::get<cache_t<std::remove_const_t<T>>*>(static_cache_set).value;
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline util::copy_const_t<traits::attribute::get_value_t<T>, T>* try_attribute() {
			if (auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.cend()) {
				return nullptr;
			} else {
				return &static_cast<const cache_t<std::remove_const_t<T>>*>(it->second)->value;
			}
		}

		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		inline std::add_const_t<traits::attribute::get_value_t<T>>* try_attribute() const {
			if (auto it = dynamic_cache_set.find(std::type_identity<T>{}); it == dynamic_cache_set.cend()) {
				return nullptr;
			} else {
				return &static_cast<const cache_t<std::remove_const_t<T>>*>(it->second)->value;
			}
		}
		#endif

		template<typename ... dep_Ts>
		void lock(priority p = priority::MEDIUM) {
			using lock_sequence = util::eval_t<traits::dependencies::get_attribute_set_t<dep_Ts...>, 
				util::sort_by_<traits::attribute::get_lock_priority, util::get_type_name>::template type
			>;
			util::apply<lock_sequence>([&]<typename ... Res_Ts>{ 
				(get_cache<Res_Ts>().acquire(p), ...); 
			});
		}

		template<typename ... dep_Ts>
		void unlock() {
			using lock_sequence = util::eval_t<traits::dependencies::get_attribute_set_t<dep_Ts...>, 
				util::sort_by_<traits::attribute::get_lock_priority, util::get_type_name>::template type,
				util::reverse
			>;
			util::apply<lock_sequence>([&]<typename ... Res_Ts>{ 
				(get_cache<Res_Ts>().release(), ...); 
			});
		}

		/* initializes an invoker service class for the event T. */
		template<traits::event_class T>
		inline ecs::invoker<T, registry<Ts...>> on() {
			return *this;
		}

		/* initializes an invoker service class for the event T. */
		template<traits::event_class T>
		inline const ecs::invoker<T, const registry<Ts...>> on() const {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T>
		inline ecs::generator<T, registry<Ts...>> generator() {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		inline ecs::generator<T, const registry<Ts...>> generator() const {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		inline ecs::pool<T, registry<Ts...>> pool() {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		inline ecs::pool<const T, const registry<Ts...>> pool() const {
			return *this;
		}
		
		/* initializes a view service class. */
		template<typename ... select_Ts, 
			typename from_T=typename traits::view_builder<select<select_Ts...>>::from_type, 
			typename where_T=typename traits::view_builder<select<select_Ts...>, from_T>::where_type>
		inline ecs::view<ecs::select<select_Ts...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			 return *this;
		}

		/* initializes a view service class. */
		template<typename ... select_Ts, 
			typename from_T=typename traits::view_builder<select<select_Ts...>>::from_type, 
			typename where_T=typename traits::view_builder<select<select_Ts...>, from_T>::where_type>
		inline ecs::view<ecs::select<const select_Ts...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return *this;
		}

		/* constructs and associates a component of type T to the entity ent. */
		template<traits::component_class T, typename ... arg_Ts>
		inline decltype(auto) emplace(traits::component::get_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().emplace_back(ent, std::forward<arg_Ts>(args)...);
		}

		/* constructs and associates a component of type T to the entity ent, if not already present else returns original. */
		template<typename seq_pol_T=ecs::policy::optimal, traits::component_class T, typename ... arg_Ts>
		inline decltype(auto) emplace_at(std::size_t idx, traits::component::get_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().emplace_at(seq_pol_T{}, idx, ent, std::forward<arg_Ts>(args)...);
		}

		/* destroys the component of type T associated to the entity ent. */
		template<typename seq_pol_T=ecs::policy::optimal, traits::component_class comp_T, traits::entity_class ent_T=entity> 		
		inline void erase(traits::entity::get_handle_t<ent_T> ent) {
			pool<comp_T>().erase(seq_pol_T{}, ent);
		}

		/* returns the number of components of type T. */
		template<traits::component_class T>
		inline std::size_t count() const {
			return pool<T>().size();
		}

		/* destroys all components of type T. */
		template<traits::component_class T>
		inline void clear() {
			pool<T>().clear();
		}

		/* returns true of entity ent has a component of type T associated. */
		template<traits::component_class T>
		inline bool has_component(traits::component::get_handle_t<T> ent) const {
			return pool<T>().contains(ent);
		}
		
		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		inline T& get_component(traits::component::get_handle_t<T> ent) {
			return pool<T>().get_component(ent);
		}

		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		inline const T& get_component(traits::component::get_handle_t<T> ent) const {
			return pool<T>().get(ent);
		}

		/* creates a new entity of type T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		inline traits::entity::get_handle_t<T> create() {
			return generator<T>().create();
		}
		
		/* destroys entity ent and all associated components. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		inline void destroy(traits::entity::get_handle_t<T> ent) {
			generator<T>().destroy(ent);
		}

		/* returns true if entity handle alive. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		inline bool alive(traits::entity::get_handle_t<T> ent) const {
			return generator<T>().alive(ent);
		}
	private:
		static_cache_set_t static_cache_set;
		#if ECS_DYNAMIC_REGISTRY
		dynamic_cache_set_t dynamic_cache_set;
		#endif
	};
}