# ECS
---
## Introduction
This is a header only library implementing the ECS architectural pattern. Inspired by:
- [entt](https://github.com/skypjack/entt)
- [ginseng](https://github.com/apples/ginseng)
- [pico_ecs](https://github.com/empyreanx/pico_headers)

### Key Features
- dynamic and static registration of component
- recursive search at compile time for dependencies
- order policies to manage sequence of component pools 
- builtin events for entity and component creation/destruction

---
## Code Example

```c++
struct position { float x, y; };
struct velocity { float x, y; };
struct health { float value; };
ecs::registry<> reg;

int main() {
	for (std::size_t i = 0; i < 64; ++i) {
		auto ent = reg.create_entity();
		reg.add_component<velocity>(ent, (i / 8) - 8, (i % 8) - 8);
		reg.add_component<position>(ent, (i / 8) * 0.5f, (i % 8) * 0.5f);
	}
	
	for (auto [pos, vel] : reg.view<position, velocity>()) {
		pos += vel;
	}

	reg.visit([&](entity e, position& pos, velocity& vel) {
		pos += vel;
	});

	auto& pool = reg.pool<health>();
	reg.visit([&](entity e, health h) { 
		if (h.value < 0.5) reg.destroy_entity(e);
	});
}
```

---
## Implementation
#### Registry
The `registry<Ts...>` class works much the same as before but the compile time dependency search has been removed. resources are cached using a type and a `consteval id` which is used as the key to a `map` storing a type erased special pointer.

##### ID
The `id` class uses the compiler `PRETTY_FUNCTION` macro extension to generate compile time a string of any type `T` and an Fx hash function generates a uuid. an `id` can be constructed from std::type_identity<T> or a constevaled `const char*`.

The `id` be constant evaluated allows it to be used in templates. eg:
`template<typename T, id ID = std::type_identity<T>>` - this template is used for the registry cache to identify a type and default to that types uuid. 

#### Pools
The `pool<T>` controls the component storage and enables component access by modeling the component-entity pairs as a sequence of elements to be reordered. This is implemented through a sparse and dense map.

specializations of `pool<T>` exist for:
- `class pool<entity>`
- `class pool<flag<V>>`
- `class pool<enum_t>`

#### Iterator
`iter<tag_Ts...>` implements a composite iteration over the components in the `registry<>`. `iter<tag_Ts...>` dereferences to proxy reference `iter_reference<tag_Ts...>` and corresponding `iter_value<tag_Ts...>`.

Tags are used to define the behaviour of the iterator using mixin specializations of `iter_mixin<Tag_T, base_T>`. Each tag mixin adds a mixin composite to the `iter<tag_Ts...>`, `iter_reference<tag_Ts...>` and `iter_value<tag_Ts...>`.

`tag_traits<tag_T>` is used to determine how the base class calls the mixin classes. `tag_traits<tag_T>` defines 5 properties: 
- **primary** - the mixin determines the base class iteration 
- **binding** - the mixin operator* is exposed in the structured binding of the reference. An unexposed mixin may still define a reference, value pair that will be updated when `iter_reference<tag_Ts...>` is moved or assigned.
- **compare** - flags the tag as comparable for reference compare operations. the iter reference comparison uses a single mixin for comparison operations.
- **ordered** - allows the `iter_mixin<tag_T, base_T>` to be used to compare the iterators. 
- **guarded** - flags the `iter_mixin<tag_T, base_T>` as being guarded meaning it tests each increment with `mixin.valid()->bool`.

`Pool<T>` uses `iter<indirect, entity, T>` as its iterator. indirect, is the index stored in the sparse map. `iter<indirect, entity, T>` will update the pool allowing stl functions such as `std::sort` or `std::shuffle`.

