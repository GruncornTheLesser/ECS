#pragma once
#include "core/traits.h"

#if ECS_DYNAMIC_REGISTRY
#include "content/id.h"
#include <unordered_map>
#endif

#include <memory>
#include <type_traits>

namespace ecs {
	template<typename reg_T>
	struct erased_cache {
		virtual void construct(reg_T& reg) = 0;
		virtual void destroy(reg_T& reg) = 0;

		virtual void acquire(reg_T& reg, priority p) = 0;
		virtual void acquire(const reg_T& reg, priority p) const = 0;
		
		virtual void release(reg_T& reg) = 0;
		virtual void release(const reg_T& reg) const = 0;
	};
	
	template<traits::attribute_class T, typename reg_T>
	struct cache final : erased_cache<reg_T>  { 
	private:
		using value_type = traits::attribute::get_value_t<T>;
		using mutex_type = traits::attribute::get_mutex_t<T>;
		
		using value_t = std::conditional_t<std::is_void_v<value_type>, std::type_identity<void>, value_type>;
		using mutex_t = std::conditional_t<std::is_void_v<mutex_type>, std::type_identity<void>, mutex_type>;
	public:
		cache() : mutex() { }
		~cache() { }
		cache(const cache&) = delete;
		cache& operator=(const cache&) = delete;
		cache(cache&& other) : value(std::move(other.value)), mutex(std::move(other.mutex)) {
		}
		cache& operator=(cache&& other) {
			if (this == &other) return *this;
			value = std::move(other.value);
			mutex = std::move(other.mutex);
			return *this;
		}

		void construct(reg_T& reg) override {
			std::construct_at(&value);

			if constexpr (requires { T::construct(reg, value); }) {
				T::construct(reg, value);
			}
		}
			
		void destroy(reg_T& reg) override {
			if constexpr (requires { T::destroy(reg, value); }) {
				T::destroy(reg, value);
			}

			std::destroy_at(&value);
		}

		void acquire(reg_T& reg, priority p) override { 
			if constexpr (requires { mutex.lock(p); }) {
				mutex.lock(p); 
			}
			else if constexpr (requires { mutex.lock(); }) {
				mutex.lock(); 
			}
		}

		void acquire(const reg_T& reg, priority p) const override { 
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
		}

		void release(reg_T& reg) override { 
			if constexpr (requires { mutex.unlock(); }) {
				mutex.unlock(); 
			}
		}

	
		void release(const reg_T& reg) const override {
			if constexpr (requires { mutex.unlock(); }) {
				mutex.unlock(); 
			}
		}
		
		union { value_t value; };
		mutable mutex_t mutex;
	};

	template<typename ... Ts> 
	class registry {	
		template<typename T> using bind_t = traits::registry_bind_t<T, registry<Ts...>>;
		
		template<typename T>
		using cache_t = ecs::cache<bind_t<T>, ecs::registry<Ts...>>;
		
		template<typename T>
		using invoker_t = ecs::invoker<util::copy_const_t<bind_t<T>, T>, util::copy_const_t<registry<Ts...>, T>>;

		template<typename T>
		using generator_t = ecs::generator<util::copy_const_t<bind_t<T>, T>, util::copy_const_t<registry<Ts...>, T>>;

		template<typename T>
		using pool_t = ecs::pool<util::copy_const_t<bind_t<T>, T>, util::copy_const_t<registry<Ts...>, T>>;

		template<typename select_T, typename from_T, typename where_T, bool is_const>
		struct get_view;

		template<typename ... select_Ts, typename from_T, typename ... where_Ts, bool is_const>
		struct get_view<select<select_Ts...>, from<from_T>, where<where_Ts...>, is_const> {
			using type = ecs::view<select<util::copy_const_t<bind_t<select_Ts>, select_Ts>...>, from<bind_t<from_T>>, where<bind_t<where_Ts>...>, std::conditional_t<is_const, const registry<Ts...>, registry<Ts...>>>;
		};

		template<typename select_T, typename from_T, typename where_T, bool is_const=false>
		using view_t = typename get_view<select_T, from_T, where_T, is_const>::type;

		template<typename select_T, typename from_T, typename where_T, bool is_const>
		struct get_rview;

		template<typename ... select_Ts, typename from_T, typename ... where_Ts, bool is_const>
		struct get_rview<select<select_Ts...>, from<from_T>, where<where_Ts...>, is_const> {
			using type = ecs::reverse_view<select<util::copy_const_t<bind_t<select_Ts>, select_Ts>...>, from<bind_t<from_T>>, where<bind_t<where_Ts>...>, std::conditional_t<is_const, const registry<Ts...>, registry<Ts...>>>;
		};

		template<typename select_T, typename from_T, typename where_T, bool is_const=false>
		using rview_t = typename get_view<select_T, from_T, where_T, is_const>::type;

		template<typename T>
		using get_component_handle_t = traits::component::get_handle_t<bind_t<T>>;

		template<typename T>
		using get_entity_handle_t = traits::entity::get_handle_t<bind_t<T>>;
	
	public:
		using static_dependencies = traits::dependencies::get_attribute_set_t<std::tuple<Ts...>, registry<Ts...>>;

		template<typename ... Us> static constexpr auto static_set_builder(std::type_identity<std::tuple<Us...>>) -> std::tuple<cache<Us, registry<Ts...>>...>;
		using static_cache_t = decltype(static_set_builder(std::type_identity<static_dependencies>{}));
		
		#if ECS_DYNAMIC_REGISTRY
		using dynamic_cache_t = std::unordered_map<id, erased_cache<registry<Ts...>>*>;
		#endif
	
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

	private:
		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, bind_t<T>>)
		util::copy_const_t<cache_t<T>, T>& get_cache() {
			return std::get<cache_t<std::remove_const_t<T>>>(static_cache);
		}

		/* returns the caches for a attribute type. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, bind_t<T>>)
		std::add_const_t<cache_t<T>>& get_cache() const {
			return std::get<cache_t<std::remove_const_t<T>>>(static_cache);
		}

		#if ECS_DYNAMIC_REGISTRY
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, bind_t<T>>)
		util::copy_const_t<cache_t<T>, T>&  get_cache() {
			return *static_cast<const cache_t<T>*>(dynamic_set.at(std::type_identity<T>{}));
		}
		
		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, bind_t<T>>)
		std::add_const_t<cache_t<T>>& get_cache() const {
			return *static_cast<const cache_t<T>*>(dynamic_set.at(std::type_identity<T>{}));
		}
		#endif

	public:	
		#if ECS_DYNAMIC_REGISTRY
		template<typename ... Us> requires (!util::pred::contains_v<static_dependencies, bind_t<Us>> && ...)
		void cache() {
			using cache_sequence = util::eval_t<traits::dependencies::get_attribute_set_t<std::tuple<Us...>, registry<Ts...>>, util::sort_by_<traits::attribute::get_init_priority>::template type>;
			
			util::apply_each<cache_sequence>([&]<typename T>{ 
				if (auto it = dynamic_set.find(std::type_identity<T>{}); it == dynamic_set.end()) {
					cache_t<T>* cache = new cache_t<T>(); // allocate cache
					cache->construct(*this);

					it = dynamic_set.emplace_hint(it, id(std::type_identity<T>{}), cache);
				}
			});
		}
		#endif

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T> requires (ECS_DYNAMIC_REGISTRY || util::pred::contains_v<static_dependencies, bind_t<T>>)
		util::copy_const_t<traits::attribute::get_value_t<bind_t<T>>, T>& get_attribute() {
			return get_cache<T>().value;
		}

		/* returns the value the attribute stored within the registry. */
		template<traits::attribute_class T> requires (ECS_DYNAMIC_REGISTRY || util::pred::contains_v<static_dependencies, bind_t<T>>)
		std::add_const_t<traits::attribute::get_value_t<bind_t<T>>>& get_attribute() const {
			return get_cache<T>().value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, bind_t<T>>)
		util::copy_const_t<traits::attribute::get_value_t<bind_t<T>>, T>* try_attribute() {
			return &get_cache<T>().value;
		}

		/* returns a pointer to the value of a attribute, if no attribute cached, returns nullptr. */
		template<traits::attribute_class T> requires (util::pred::contains_v<static_dependencies, bind_t<T>>)
		std::add_const_t<traits::attribute::get_value_t<bind_t<T>>>* try_attribute() const {
			return &get_cache<T>().value;
		}

		template<traits::attribute_class T> requires (!util::pred::contains_v<static_dependencies, bind_t<T>>)
		util::copy_const_t<traits::attribute::get_value_t<bind_t<T>>, T>* try_attribute() {
			#if ECS_DYNAMIC_REGISTRY
			auto it = dynamic_set.find(std::type_identity<T>{}); 
			if (it != dynamic_set.cend()) {
				return &static_cast<const cache_t<std::remove_const_t<T>>*>(it->second)->value;
			}
			#endif
			return nullptr;
		}	

		template<typename ... Us> requires (ECS_DYNAMIC_REGISTRY || (util::pred::contains_v<static_dependencies, bind_t<Us>> && ...))
		void lock(priority p = priority::MEDIUM) {
			using lock_sequence = util::eval_t<traits::dependencies::get_attribute_set_t<std::tuple<Us...>, registry<Ts...>>, util::sort_by_<traits::attribute::get_lock_priority, util::get_type_name>::template type, util::reverse>;
			util::apply<lock_sequence>([&]<typename ... Res_Ts>{
				(get_cache<Res_Ts>().acquire(p), ...); 
			});
		}

		template<typename ... Us> requires (ECS_DYNAMIC_REGISTRY || (util::pred::contains_v<static_dependencies, bind_t<Us>> && ...))
		void unlock() {
			using lock_sequence = util::eval_t<traits::dependencies::get_attribute_set_t<std::tuple<Us...>, registry<Ts...>>, util::sort_by_<traits::attribute::get_lock_priority, util::get_type_name>::template type>;
			util::apply<lock_sequence>([&]<typename ... Res_Ts>{ 
				(get_cache<Res_Ts>().release(), ...); 
			});
		}

		/* initializes an invoker service class for the event T. */
		template<traits::event_class T>
		invoker_t<T> on() {
			return *this;
		}

		/* initializes an invoker service class for the event T. */
		template<traits::event_class T>
		invoker_t<const T> on() const {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T>
		generator_t<T> generator() {
			return *this;
		}

		/* initializes a generator service class for the entity T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		generator_t<const T> generator() const {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		pool_t<T> pool() {
			return *this;
		}

		/* initializes a pool service class for the component T. */
		template<traits::component_class T>
		pool_t<const T> pool() const {
			return *this;
		}
		
		/* initializes a view service class. */
		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename ... where_Ts>
		view_t<ecs::select<select_Ts...>, from_T, ecs::where<where_Ts...>>
		view(from_T from={}, where_Ts&& ... where) {
			return *this;
		}

		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename ... where_Ts>
		rview_t<ecs::select<select_Ts...>, from_T, ecs::where<where_Ts...>>
		rview(from_T from={}, where_Ts&& ... where) {
			return *this;
		}

		/* initializes a view service class. */
		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename ... where_Ts>
		view_t<ecs::select<const select_Ts...>, from_T, ecs::where<where_Ts...>, true>
		view(from_T from={}, where_Ts&& ... where) const {
			return *this;
		}

		template<typename ... select_Ts, typename from_T=from<util::find_t<std::tuple<select_Ts...>, traits::is_component>>, typename ... where_Ts>
		rview_t<ecs::select<const select_Ts...>, from_T, ecs::where<where_Ts...>, true>
		rview(from_T from={}, where_Ts&& ... where) const {
			return *this;
		}

		/* constructs and associates a component of type T to the entity ent. */
		template<traits::component_class T, typename ... arg_Ts>
		decltype(auto) emplace(get_component_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().template emplace_back(ent, std::forward<arg_Ts>(args)...);
		}

		/* constructs and associates a component of type T to the entity ent, if not already present else returns original. */
		template<traits::component_class T, typename seq_T=policy::optimal, typename ... arg_Ts>
		decltype(auto) emplace_at(std::size_t idx, get_component_handle_t<T> ent, arg_Ts&& ... args) {
			return pool<T>().template emplace_at<seq_T>(idx, ent, std::forward<arg_Ts>(args)...);
		}

		/* destroys the component of type T associated to the entity ent. */
		template<traits::component_class comp_T, typename seq_T=policy::optimal>
		void erase(get_component_handle_t<comp_T> ent) {
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
		bool has_component(get_component_handle_t<T> ent) const {
			return pool<T>().contains(ent);
		}
		
		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		decltype(auto) get_component(get_component_handle_t<T> ent) {
			return pool<T>().get_component(ent);
		}

		/* returns the component of type T associated with the entity ent. */
		template<traits::component_class T>
		decltype(auto) get_component(get_component_handle_t<T> ent) const {
			return pool<T>().get(ent);
		}

		/* creates a new entity of type T. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY, typename ... arg_Ts>
		get_entity_handle_t<T> create(arg_Ts&& ... args) {
			return generator<T>().create(std::forward<arg_Ts>(args)...);
		}
		
		/* destroys entity ent and all associated components. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		void destroy(get_entity_handle_t<T> ent) {
			generator<T>().destroy(ent);
		}

		/* returns true if entity handle alive. */
		template<traits::entity_class T=ECS_DEFAULT_ENTITY>
		bool alive(get_entity_handle_t<T> ent) const {
			return generator<T>().alive(ent);
		}
	private:
		static_cache_t static_cache;
		#if ECS_DYNAMIC_REGISTRY
		dynamic_cache_t dynamic_set;
		#endif
	};
}