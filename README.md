# ECS
---
## Introduction
This is a header only library implementing the ECS architectural pattern. Inspired by:
- [entt](https://github.com/skypjack/entt)
- [ginseng](https://github.com/apples/ginseng)
- [pico_ecs](https://github.com/empyreanx/pico_headers)

### Key Features
- customizable behaviour for attributes, entities, components and events
- dynamic and static registration of component
- recursive search at compile time for dependencies
- order policies to manage sequence of component pools 
- builtin events for entity and component creation/destruction

---
## Code Example

```c++
struct position {
	float x, y;
};

struct velocity {
	float x, y;
};

ecs::registry<position, velocity> reg;

int main() {
	for (std::size_t i = 0; i < 64; ++i) {
		auto hnd = reg.create();
		reg.emplace<velocity>(hnd, (i / 8) - 8, (i % 8) - 8);
		reg.emplace<position>(hnd, (i / 8) * 0.5f, (i % 8) * 0.5f);
	}
	
	while (true) {
		for (auto [pos, vel] : reg.view<position, velocity>()) {
			pos += vel;
		}
	}
}
```

<details>
  <summary>Attribute</summary>
  
```c++
struct basic_attribute {
	using ecs_category = ecs::tag::attribute;
	using mutex_type = void; // disables locking feature

	// static constexpr int lock_priority = ...; // high lock priority gets locked first
	// static constexpr int init_priority = ...; // high init priority gets contructed first
	using value_type = std::vector<char>; // the value of the attribute
	using dependency_set = std::tuple<ecs::entity>; // additional dependencies

	static inline void construct(auto& reg, auto& attrib) {
		attrib.resize(256, ' ');
	}

	static inline void destroy(auto& reg, auto& attrib) {
		attrib.clear();
	}
};

```

- `reg.cache<basic_attribute>();` - dynamically adds dependencies to registry
- `auto* try_ptr = reg.try_attribute_t<basic_attribute>();` - gets a pointer to an attribute if it exists, else returns nullptr
- `auto& attrib = reg.get_attribute<basic_attribute>();` - returns a reference to an attribute, throws error if not exists
</details>

<details>
  <summary>Custom Entity</summary>
  
```c++
struct basic_entity {
	using ecs_category = ecs::tag::entity;
	using integral_type = uint64_t; // custom handle integral type
	static constexpr std::size_t version_width = 16; // number of bits storing the width
	// using create_event = ...; // event called on entity creation
	// using destroy_event = ...; // event called on entity destruction
	// using dependency_set = ...; // additional dependencies
};

```

- `auto hnd = reg.create<basic_entity>();` - creates new handle and emits create event
- `reg.alive(hnd);` - returns true if handle alive
- `reg.destroy<basic_entity>();` - destroys old handle and emits destroy event
</details>

<details>
  <summary>Custom Event</summary>

```c++
struct basic_event {
	using ecs_category = ecs::tag::event;
	using callback_type = void(int, int);
	using listener_type = ecs::listener<basic_event>;
	using dependency_set = std::tuple<>;
};

template<typename reg_T=ecs::registry<>>
struct registry_bound_event {
	using ecs_category = ecs::tag::event;
	
	template<typename reg_U> 
	using rebind_registry = registry_bound_event<reg_U>;
	
	using callback_type = void(reg_T&);

	// using listener_type = ...; // custom listener type
	// using dependency_set = ...; // additional dependencies
};

```

```c++
auto event_listener_lambda = [](int, int) { /*...*/ };
void event_listener_func(int, int) { /*...*/ }
int main() {
	reg.on<basic_event>().attach(event_listener_lambda);
	reg.on<basic_event>().attach(event_listener_func);
	reg.on<basic_event>().invoke(20, 25);
	reg.on<basic_event>().detach(event_listener_lambda);
	reg.on<basic_event>().detach(event_listener_func);

	// or alternatively...
	reg.on<basic_event>() += event_listener_func; // add 
	reg.on<basic_event>() -= event_listener_func; // remove
}

```

> [!WARNING]
> a default listener_type must not declare its own dependencies or else create an infinite dependency chain often causing a code editor to crash.

</details>

<details>
  <summary>Custom Component</summary>
  

```c++
struct basic_component {
	// no ecs_category required as defaults to component
	using value_type = int;
	using entity_type = basic_entity;
	using initialize_event = basic_event;
	// using terminate_event = ...;
	// static constexpr std::size_t page_size = ...;
};
```
> [!WARNING]
> when value type `is_void` or `is_empty` the component storage is disabled.
</details>

---
## Implementation
### Registry
```c++
using dynamic_registry_type = registry<>; // dynamic registry
using static_registry_type = registry<basic_component, basic_event, basic_attribute>;
```
The registry searches for the dependencies of the declared types at compile time and stores a tuple of the attributes found. This is the main purpose of the metaprogramming library `tuple_util` to search for new dependencies and prevent circular searches and code editor crashes.

Any undeclared dependencies not included from the registry definition can be dynamically cached. dynamically cached attributes are implemented using `erased_cache_t`, a virtual class to implement construction, destruction and locking behaviours. The `erased_cache_t` is stored in an unordered map using `id` as a key:
- `reg.cache<Ts...>()` - adds new attributes along with all attribute dependencies.
- `reg.get_attribute<T>()` - returns a ref to an existing attribute.
- `reg.try_attribute<T>()` - returns a ptr to an existing attribute else returns 

registry also supports attribute access locks. A set of attributes can be locked with no deadlocks garanteed with a compile time sort algorithm:
- `reg.lock<Ts...>()` - locks a set of types and their dependencies.
- `reg.lock<Ts...>()` - unlocks a set of types and their dependencies. 

> [!NOTE]
> attribute access locks are in anticipation of a pipeline service class.
#### Services
services store a reference to a registry to retrieve attributes as necessary and perform specialized operations. Common methods are added to the registry for convenience. There currently exist 4 service classes:
- `class generator<entity_class, registry_type>`
- `class pool<component_class, registry_type>`
- `class invoker<event_class, registry_type>`
- `class view<select_type, from_type, where_type, registry_type>`

#### Generator
```c++
auto gen = reg.generator<basic_entity>();
```
The `generator<entity_class>` is used for entity handle creation. This is primarily through the `factory<entity_class>` attribute that implements a `sparse_list<handle_type>`. A `sparse_list<handle_type>` embeds a linked list inside a sparse container to allow sparse lookups of valid values and a linked list of destroyed handles. 

> [!WARNING]
> handle creation will likely be updated in future and is subject to change...

#### Pool
```c++
auto pool = reg.pool<basic_component>();
```
The `pool<component_class>` controls the component storage and enables component access by modeling the component-entity pairs as a sequence of elements to be reordered. This is implemented through 3 attributes:
- `manager<comp_T>` - an attribute with a vector-like value storing the entity handles in a random access container. By default this is implemented with `packed<handle_type>`.
- `indexer<comp_T>` - an attribute with a map-like value. stores the lookup for entity-index to index. By default this is implemented with `sparse<std::size_t>`
- `storage<comp_T>` - an attribute with a vector-like value stioring the component values in a random access container. By default this is implemented with `packed<value_type>`.


There are 4 primary modifier methods for adding and removing components from their entities while also controlling the sequence of component entity pairs inside of the pool.
- `pool.emplace_back(ent, ...)` - inserts a component-entity pair at the back of the pool.
- `pool.emplace_at<policy>(ind, ent, ...)` - inserts a component-entity pair at the index.
- `pool.erase<policy>(ent)` - erases a component-entity pair.
- `pool.erase_at<policy>(ind)` - erases the component-entity at the index.

##### Policy
Where possible a policy argument can be passed which determine how the container is reordered to accommodate the change. The library currently supports 2 policy types: 
- `strict` - maintains order of all elements within the pool. $O(n)$
- `optimal` - swaps elements with the back of the pool. $O(1)$ 

> [!NOTE]
> further sequence policies may be implemented for grouped behaviours. eg for grouped component types.

> [!NOTE]
> currently only sequence policies are implemented. Further policies may be added to define execution time ie immediate vs deferred execution. 

#### View
```c++
auto view = reg.view<ecs::entity, A>(ecs::from<B>{}, ecs::inc<C, D, E>{});
```
The view class supports iteration and filtering of components using direct and indirect access. the view class is built from `select<class...>`, `from<component_class>` and `where<class...>`. Where possible components are accessed *directly* using the shared index of their position and the component defined using `from<component_class>`. Else the components are accessed *indirectly* using the `indexing<component_class>` attribute to find the correct position.

The `view_iterator` on each increment simply executes a forward search over the handles of the `from<component_class>` to find the next valid index that meets the query. A component in `select<Ts...>` automatically is tested for inclusion and its iterator is cached for faster dereferencing.

```c++ 
for (auto [ent, a, b, c] : reg.view<ecs::entity, A, const B, C>()) { }
for (auto [ent] : reg.view<ecs::entity>(std::from<A>{}, exc<A>{}, inc<B>{})) { }
for (auto [ent, a] : reg.rview<ecs::entity, A>()) { } // reverse
```

#### Invoker
```c++
auto invk = reg.on<basic_event>();
```

The invoker is simply a shallow wrapper around a `listener<event_class>` component. The `listener<event_class>` declares a void entity_type meaning its unassociated with any entity type. The `listener<event_class>` instead uses a pointer of the callback_type as a handle. The invoker only accepts stateless lambdas meaning a lamda passed must have an empty capture set.

```c++
void event_listener_func(auto& val) { ++val; }
auto event_listener_lamda = [](auto& val){ ++val; };

invk.attach(event_listener_lamda);
invk.attach(event_listener_func);
invk.invoke(event_data);
invk.detach(event_listener_lamda);
invk.detach(event_listener_func);
```

The `invoker<entity_class>` also supports short hand operators:
```c++
invk += event_listener; // attach
invk -= event_listener; // detach
```

> [!INFO]
> a fire once flag feature is planned. This would automatically remove an entity 

### Traits
there are various traits for the different categories which can optionally be defined in a class for specialized behaviour. there are two trait metafunctions: 
- `get_[NAME]_t` 
  : returns the type defined in the corresponding traits class. This is the standard getter function for declared types. For example to get the handle_type of a component you can use:
    ```c++
	using handle_type = ecs::traits::component::get_handle_t<basic_component>;
	```
- `get_trait_[NAME]_t`
  : returns the type defined within a class. This is used to get a declared type or default to a different class. For example to get the ecs_category:
    ```c++
    using ecs_category = ecs::traits::get_trait_category_t<basic_component, ECS_DEFAULT_TAG>;
    ```
#### TAG
A tag defines the category and can be used to define a class of behaviours and defaults. The category is used to distinguish between the various class types used in the library. A `c++20` concept exists for each category to statically assert the correct class is passed.

Custom tags are implemented using the trait getter classes. The category trait classes define the getting behaviour to first default to a possible declaration in the class type, else default to a possible declaration in the tag type and then finally to a global default. For example:
```c++
using entity_tag_1 = ecs::tag::entity;
struct entity_tag_2 : ecs::tag::entity { using create_event = ...; };
struct entity_tag_3 : ecs::tag::entity { };

struct entity_1 { 
	using ecs_category = entity_tag_1;
	using create_event = ...;
};
struct entity_2 { 
	using ecs_category = entity_tag_2; 
};
struct entity_3 { 
	using ecs_category = entity_tag_3; 
};

using create_event_1 = get_create_event_t<entity_1>; // entity_1::create_event
using create_event_2 = get_create_event_t<entity_2>; // entity_tag_2::create_event
using create_event_3 = get_create_event_t<entity_3>; // ECS_DEFAULT_CREATE_EVENT
```
#### Macros
These macros can be defined to control the behaviour of the library.

- `ECS_RECURSIVE_DEPENDENCY`
  : Allows the disabling of the registry's recursive search for dependencies. You might disable this for performance reasons. Defaults to `true`.

- `ECS_DYNAMIC_REGISTRY`
  : Allows the disabling of the dynamic caching features of the registry. You might  disable this as personal preference to enforce only static storage of attributes. Defaults to `true`.

- `ECS_DEFAULT_TAG`
  : Determines the default behaviour when no `ecs_category` is defined in a class. Defaults to `ecs::tag::component`.

- `ECS_DEFAULT_HANDLE_INTEGRAL`
  : Determines the default handle integral_type for an entity. Defaults to `uint32_t`.

- `ECS_DEFAULT_HANDLE_VERSION_WIDTH`
  : Determines the default version width in bits dedicated to storing the version data of a handle. Defaults to `12`.

- `ECS_DEFAULT_PAGE_SIZE`
  : Determines the default page size of Defaults to `4096`.

- `ECS_DEFAULT_MUTEX`
  : Determines the default mutex type for attributes. A common use case might be to disable locking altogether using `void`. Defaults to `ecs::priority_mutex`.

- `ECS_DEFAULT_ENTITY`
  : Determines the default entity table components belong to. Defaults to `ecs::entity`.

- `ECS_DEFAULT_INITIALIZE_EVENT`
  : Determines the default initialize event called on the emplacement of a new component. A common use case might be to disable initialize events using `void`. Defaults to `ecs::event::initialize<T>`.

- `ECS_DEFAULT_TERMINATE_EVENT`
  : Determines the default terminate event called on the erasure of a component. A common use case might be to disable terminate events using `void`. Defaults to `ecs::event::terminate<T>`.

- `ECS_DEFAULT_CREATE_EVENT`
  : Determines the default create event called on the creation of a new entity handle. A common use case might be to disable create events using `void`. Defaults to `ecs::event::create<T>`.

- `ECS_DEFAULT_DESTROY_EVENT`
  : Determines the default destroy event called on the destruction of a entity handle. A common use case might be to disable destroy events using `void`. Defaults to `ecs::event::destroy<T>`.

- `ECS_DEFAULT_ACQUIRE_EVENT`
  : Determines the default event called on the locking of an attribute. A common use case might be to disable acquire events using `void`. Defaults to `ecs::event::acquire<T>`.

- `ECS_DEFAULT_RELEASE_EVENT`
  : Determines the default event called on the unlocking of an attribute. A common use case might be to disable release events using `void`. Defaults to `ecs::event::release<T>`.
