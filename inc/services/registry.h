#pragma once
#include "core/traits.h"

#if ECS_DYNAMIC_REGISTRY
#include "content/attrib_id.h"
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
			using mutex_type = util::eval_if_t<traits::attribute::get_mutex_t<T>, std::is_void, util::wrap_<std::type_identity>::template type>;
			
			using acquire_event = traits::attribute::get_acquire_event_t<T>;
			using release_event = traits::attribute::get_release_event_t<T>;
			
			static constexpr bool acquire_enabled = !std::is_void_v<acquire_event>;
			static constexpr bool release_enabled = !std::is_void_v<release_event>;

			cache_t() : mutex() { }
			~cache_t() { }
			cache_t(const cache_t&) = delete;
			cache_t& operator=(const cache_t&) = delete;
			cache_t(cache_t&& other) : value(std::move(other.value)), mutex(std::move(other.mutex)) {
			}
			cache_t& operator=(cache_t&& other) {
				if (this == &other) return *this;
				value = std::move(other.value);
				mutex = std::move(other.mutex);
				return *this;
			}

			void construct(registry<Ts...>& reg) override {
				std::construct_at(&value);

				if constexpr (requires { T::construct(reg, value); }) {
					T::construct(reg, value);
				}
			}
				
			void destroy(registry<Ts...>& reg) override {
				if constexpr (requires { T::destroy(reg, value); }) {
					T::destroy(reg, value);
				}

				std::destroy_at(&value);
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
				if constexpr (requires { mutex.unlock(); }) {
					mutex.unlock(); 
				}

				if constexpr (release_enabled) {
					reg.template on<release_event>().invoke(value);
				}
			}
			
			union { value_type value; };
			mutable mutex_type mutex;
		};
	public:
		using static_dependencies = traits::dependencies::get_attribute_set_t<Ts...>;
		using static_set_type = util::eval_each_t<static_dependencies, util::wrap_<cache_t>::template type>;
		
		#if ECS_DYNAMIC_REGISTRY
		using dynamic_set_type = std::unordered_map<attrib_id, erased_cache_t*>;
		#endif
	public:
		registry() {
			using init_sequence = util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>;
			util::apply<init_sequence>([&]<typename ... attrib_Ts>() { 
				(get_cache<attrib_Ts>().construct(*this), ...);
			});
		};
		~registry() {
			#if ECS_DYNAMIC_REGISTRY
			for (auto& [id, cache] : dynamic_set) { 
				cache->destroy(*this);
				delete cache;
			}
			#endif
			
			using term_sequence = util::reverse_t<util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>>;
			util::apply<term_sequence>([&]<typename ... attrib_Ts>() { 
				(get_cache<attrib_Ts>().destroy(*this), ...);
			});
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
			dynamic_set = std::move(other.dynamic_set);
			#endif
		}
		registry& operator=(registry&& other) { 
			if (this == &other) return *this;

			using move_sequence = util::sort_by_t<static_dependencies, traits::attribute::get_init_priority>;
			util::apply<move_sequence>([&]<typename ... attrib_Ts>() { 
				(std::move(get_cache<attrib_Ts>(), other.get_cache<attrib_Ts>()), ...);
			});

			#if ECS_DYNAMIC_REGISTRY
			for (auto& [id, cache] : dynamic_set) { 
				cache->destroy(*this);
				delete cache;
			}
			dynamic_set = std::move(other.dynamic_set);
			#endif

			return *this;
		}

		#if ECS_DYNAMIC_REGISTRY
		template<typename ... Us> requires (!traits::is_attribute_v<Ts> && ...)
		void cache() {
			using cache_sequence = util::eval_t<
				traits::dependencies::get_attribute_set_t<Ts...>, 
				util::sort_by_<traits::attribute::get_init_priority>::template type,
				util::filter_<util::pred::element_of_<static_dependencies>::template type>::template type
			>;
			
			util::apply_each<cache_sequence>([&]<typename T>{ 
				if (auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.end()) {
					cache_t<T>* cache = new cache_t<T>(); // allocate cache
					cache->construct(*this);
	
					it = dynamic_set.emplace_hint(it, attrib_id(std::type_identity<T>{}), cache);
				}
			});
		}
		#endif

		/* caches a attribute, creates a new attribute cache if one isnt already cached. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>) 
		cache_t<T>& cache() {
			return std::get<cache_t<T>>(static_set);
		}
		
		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>) 
		cache_t<T>& cache() {
			if (auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.end()) {
				cache_t<T>* cache = new cache_t<T>(); // allocate cache
				cache->construct(*this);

				it = dynamic_set.emplace_hint(it, attrib_id(std::type_identity<T>{}), cache);
			}
			else {
				return *static_cast<cache_t<T>*>(it->second);
			}
		}
		#endif
		
		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<cache_t<std::remove_const_t<T>>, T>& get_cache() {
			return std::get<cache_t<std::remove_const_t<T>>>(static_set);
		}

		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<cache_t<std::remove_const_t<T>>>& get_cache() const {
			return std::get<cache_t<std::remove_const_t<T>>>(static_set);
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<cache_t<std::remove_const_t<T>>, T>&  get_cache() {
			return *static_cast<const cache_t<T>*>(dynamic_set.at(std::type_identity<T>{}));
		}
		
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<cache_t<std::remove_const_t<T>>>& get_cache() const {
			return *static_cast<const cache_t<T>*>(dynamic_set.at(std::type_identity<T>{}));
		}
		#endif

		/* returns a pointer to a attribute type if cached. if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<cache_t<std::remove_const_t<T>>, T>* try_cache() {
			return &std::get<cache_t<std::remove_const_t<T>>>(static_set);
		}

		/* returns a pointer to a attribute type if cached. if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<cache_t<std::remove_const_t<T>>>* try_cache() const {
			return &std::get<cache_t<std::remove_const_t<T>>>(static_set);
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<cache_t<std::remove_const_t<T>>, T>* try_cache() {
			if(auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.cend()) {
				return nullptr;
			} 
			else {
				return static_cast<cache_t<std::remove_const_t<T>>*>(it->second);
			}
		}
		
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<cache_t<std::remove_const_t<T>>>* try_cache() const {
			if(auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.cend()) {
				return nullptr;
			} else {
				return static_cast<const cache_t<std::remove_const_t<T>>*>(it->second);
			}
		}
		#endif

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T>
		util::copy_const_t<traits::attribute::get_value_t<T>, T>& get_attribute() {
			return get_cache<T>().value;
		}

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T>
		std::add_const_t<traits::attribute::get_value_t<T>>& get_attribute() const {
			return get_cache<T>().value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<traits::attribute::get_value_t<T>, T>* try_attribute() {
			return &std::get<cache_t<std::remove_const_t<T>>*>(static_set).value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<traits::attribute::get_value_t<T>>* try_attribute() const {
			return &std::get<cache_t<std::remove_const_t<T>>*>(static_set).value;
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		util::copy_const_t<traits::attribute::get_value_t<T>, T>* try_attribute() {
			if (auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.cend()) {
				return nullptr;
			} else {
				return &static_cast<const cache_t<std::remove_const_t<T>>*>(it->second)->value;
			}
		}

		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, std::remove_const_t<T>>)
		std::add_const_t<traits::attribute::get_value_t<T>>* try_attribute() const {
			if (auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.cend()) {
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
		ecs::invoker<T, registry<Ts...>> on() {
			return *this;
		}

		/* initializes an invoker service class for the event T. */
		template<traits::event_class T>
		const ecs::invoker<T, const registry<Ts...>> on() const {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T>
		ecs::generator<T, registry<Ts...>> generator() {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		ecs::generator<T, const registry<Ts...>> generator() const {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		ecs::pool<T, registry<Ts...>> pool() {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		ecs::pool<const T, const registry<Ts...>> pool() const {
			return *this;
		}
		
		/* initializes a view service class. */
		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename where_T=ecs::where<>>
		ecs::view<ecs::select<select_Ts...>, from_T, where_T, ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) {
			return *this;
		}

		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename where_T=ecs::where<>>
		ecs::reverse_view<ecs::select<select_Ts...>, from_T, where_T, ecs::registry<Ts...>>
		rview(from_T from={}, where_T where={}) {
			return *this;
		}

		/* initializes a view service class. */
		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename where_T=ecs::where<>>
		ecs::view<ecs::select<const select_Ts...>, from_T, where_T, const ecs::registry<Ts...>>
		view(from_T from={}, where_T where={}) const {
			return *this;
		}

		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename where_T=ecs::where<>>
		ecs::reverse_view<ecs::select<const select_Ts...>, from_T, where_T, const ecs::registry<Ts...>>
		rview(from_T from={}, where_T where={}) const {
			return *this;
		}

		/* constructs and associates a component of type T to the entity ent. */
		template<traits::component_class T, typename ... arg_Ts>
		decltype(auto) emplace(traits::component::get_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().template emplace_back(ent, std::forward<arg_Ts>(args)...);
		}

		/* constructs and associates a component of type T to the entity ent, if not already present else returns original. */
		template<traits::component_class T, typename seq_T=policy::optimal, typename ... arg_Ts>
		decltype(auto) emplace_at(std::size_t idx, traits::component::get_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().template emplace_at<seq_T>(idx, ent, std::forward<arg_Ts>(args)...);
		}

		/* destroys the component of type T associated to the entity ent. */
		template<traits::component_class comp_T, typename seq_T=policy::optimal>
		void erase(traits::component::get_handle_t<comp_T> ent) {
			pool<comp_T>().template erase<seq_T>(ent);
		}

		/* returns the number of components of type T. */
		template<traits::component_class T>
		std::size_t count() const {
			return pool<T>().size();
		}

		/* destroys all components of type T. */
		template<traits::component_class T>
		void clear() {
			pool<T>().clear();
		}

		/* returns true of entity ent has a component of type T associated. */
		template<traits::component_class T>
		bool has_component(traits::component::get_handle_t<T> ent) const {
			return pool<T>().contains(ent);
		}
		
		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		decltype(auto) get_component(traits::component::get_handle_t<T> ent) {
			return pool<T>().get_component(ent);
		}

		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		decltype(auto) get_component(traits::component::get_handle_t<T> ent) const {
			return pool<T>().get(ent);
		}

		/* creates a new entity of type T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY, typename ... arg_Ts>
		traits::entity::get_handle_t<T> create(arg_Ts&& ... args) {
			return generator<T>().create(std::forward<arg_Ts>(args)...);
		}
		
		/* destroys entity ent and all associated components. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		void destroy(traits::entity::get_handle_t<T> ent) {
			generator<T>().destroy(ent);
		}

		/* returns true if entity handle alive. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		bool alive(traits::entity::get_handle_t<T> ent) const {
			return generator<T>().alive(ent);
		}
	private:
		static_set_type static_set;
		#if ECS_DYNAMIC_REGISTRY
		dynamic_set_type dynamic_set;
		#endif
	};
}