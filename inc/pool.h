#pragma once
#include "fwd.h"
#include <algorithm>
#include <memory>

namespace ecs {
	template<typename T>
	class pool {
	public:	
		using iterator = iter<indirect, entity, T>;
		using const_iterator = iter<const indirect, const entity, const T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using reference = iterator::reference;
		using const_reference = const_iterator::reference;
		
		constexpr pool() noexcept;
		constexpr ~pool() noexcept;

		constexpr pool(pool&& other) noexcept;
		constexpr pool& operator=(pool&& other) noexcept;

		template<typename U> constexpr U& at(std::size_t pos);
		template<typename U> constexpr const U& at(std::size_t pos) const;
		template<typename U> constexpr U& back();
		template<typename U> constexpr const U& back() const;
		template<typename U> constexpr U& front();
		template<typename U> constexpr const U& front() const;

		constexpr reference at(std::size_t pos);
		constexpr const_reference at(std::size_t pos) const;
		constexpr reference back();
		constexpr const_reference back() const;
		constexpr reference front();
		constexpr const_reference front() const;

		constexpr void clear();

		[[nodiscard]] constexpr std::size_t capacity() const { return page_extent * page_size; }
		[[nodiscard]] constexpr std::size_t size() const { return extent; }
		[[nodiscard]] constexpr bool contains(const entity& ent) const;
		
		template<typename ... Ts> constexpr iter<Ts...> begin() noexcept;
		template<typename ... Ts> constexpr iter<Ts...> end() noexcept;
		
		template<typename ... Ts> constexpr iter<const Ts...> begin() const noexcept;
		template<typename ... Ts> constexpr iter<const Ts...> end() const noexcept;
		
		template<typename ... Ts> constexpr std::reverse_iterator<iter<Ts...>> rbegin() noexcept;
		template<typename ... Ts> constexpr std::reverse_iterator<iter<Ts...>> rend() noexcept;
		
		template<typename ... Ts> constexpr std::reverse_iterator<iter<const Ts...>> rbegin() const noexcept;
		template<typename ... Ts> constexpr std::reverse_iterator<iter<const Ts...>> rend() const noexcept;

		template<typename ... Ts> constexpr iter<Ts...> find(const entity& ent);
		template<typename ... Ts> constexpr iter<const Ts...> find(const entity& ent) const;
		
		constexpr iterator begin() noexcept { return begin<indirect, entity, T>(); }
		constexpr iterator end() noexcept { return end<indirect, entity, T>(); }
		
		constexpr const_iterator begin() const noexcept { return begin<indirect, entity, T>(); }
		constexpr const_iterator end() const noexcept { return end<indirect, entity, T>(); }
		
		constexpr reverse_iterator rbegin() noexcept { return rbegin<indirect, entity, T>(); }
		constexpr reverse_iterator rend() noexcept { return rend<indirect, entity, T>(); }
		
		constexpr const_reverse_iterator rbegin() const noexcept { return rbegin<indirect, entity, T>(); }
		constexpr const_reverse_iterator rend() const noexcept { return rend<indirect, entity, T>(); }

		constexpr iterator find(const entity& ent) { return find<indirect, entity, T>(ent); }
		constexpr const_iterator find(const entity& ent) const { return find<indirect, entity, T>(ent); }
				
		template<typename ... arg_Ts>
		constexpr T& emplace_back(const entity& ent, arg_Ts&& ... args);
		
		template<std::input_iterator It, typename ... arg_Ts>
		constexpr void emplace_back(It first, It last, arg_Ts&& ... args);
		
		template<typename ... arg_Ts>
		constexpr T& emplace(const_iterator it, const entity& ent, arg_Ts&& ... args); 

		template<std::input_iterator It, typename ... arg_Ts>
		constexpr void emplace(const_iterator it, It first, It last, arg_Ts&& ... args);

		constexpr bool erase(const entity& ent);

		constexpr void erase(const_iterator it);
		constexpr void erase(const_iterator first, const_iterator last);

		constexpr void pop_back();

		constexpr void reserve(std::size_t n);

		constexpr void shrink_to_fit();

		template<typename U> constexpr U** data();
		template<typename U> constexpr U const* const* data() const;		
	private:
		constexpr ecs::indirect& assure(const ecs::entity& ent);

		entity**   manager = nullptr;
		indirect** indexer = nullptr;
		T**        storage = nullptr;
		
		index_t      extent = 0; 			// number of elements stored in pool
		index_t      page_extent = 0; 		// number of allocated pages
		index_t      sparse_capacity = 8; 	// number of page indexer allocated for sparse lookup
		index_t      packed_capacity = 8; 	// number of page indexes allocated for packed storage
	};
}

template<typename T>
constexpr ecs::pool<T>::pool() noexcept {
	manager = std::allocator<entity*>{}.allocate(packed_capacity);
	indexer = std::allocator<indirect*>{}.allocate(sparse_capacity);
	storage = std::allocator<T*>{}.allocate(packed_capacity);
	std::fill_n(storage, packed_capacity, nullptr);
	
	extent = 0;
	std::fill_n(manager, packed_capacity, nullptr);
	std::fill_n(indexer, sparse_capacity, nullptr);
}

template<typename T>
constexpr ecs::pool<T>::~pool() noexcept {
	if (manager != nullptr) {
		clear();

		for (std::size_t i = 0; i < page_extent; ++i) {
			std::allocator<entity>{}.deallocate(manager[i], page_size);
			std::allocator<T>{}.deallocate(storage[i], page_size);
		}

		for (std::size_t i = 0; i < sparse_capacity; ++i) {
			if (indexer[i] == nullptr) continue;
			std::allocator<indirect>{}.deallocate(indexer[i], page_size);
		}

		std::allocator<entity*>{}.deallocate(manager, packed_capacity);
		std::allocator<indirect*>{}.deallocate(indexer, sparse_capacity);
		std::allocator<T*>{}.deallocate(storage, packed_capacity);
	}
}

template<typename T>
constexpr ecs::pool<T>::pool(pool&& other) noexcept
 : manager(other.manager), indexer(other.indexer), storage(other.storage), extent(other.extent), page_extent(other.page_extent), packed_capacity(other.packed_capacity), sparse_capacity(other.sparse_capacity) {
	other.manager = nullptr;
	other.indexer = nullptr;
	other.storage = nullptr;
	other.extent = 0;
	other.page_extent = 0;
	other.sparse_capacity = 0;
	other.packed_capacity = 0;
}

template<typename T>
constexpr ecs::pool<T>& ecs::pool<T>::operator=(ecs::pool<T>&& other) noexcept {
	if (this == &other) return *this;
	
	if (manager != nullptr) {
		clear();

		for (std::size_t i = 0; i < page_extent; ++i) {
			std::allocator<entity>{}.deallocate(manager[i], page_size);
			std::allocator<T>{}.deallocate(storage[i], page_size);
		}

		for (std::size_t i = 0; i < sparse_capacity; ++i) {
			if (indexer[i] == nullptr) continue;
			std::allocator<indirect>{}.deallocate(indexer[i], page_size);
		}

		std::allocator<entity*>{}.deallocate(manager, packed_capacity);
		std::allocator<indirect*>{}.deallocate(indexer, sparse_capacity);
		std::allocator<T*>{}.deallocate(storage, packed_capacity);
	}
	
	manager = other.manager;
	indexer = other.indexer;
	storage = other.storage;
	extent = other.extent;
	page_extent = other.page_extent;
	sparse_capacity = other.sparse_capacity;
	packed_capacity = other.packed_capacity;

	other.manager = nullptr;
	other.indexer = nullptr;
	other.storage = nullptr;
	other.extent = 0;
	other.page_extent = 0;
	other.sparse_capacity = 0;
	other.packed_capacity = 0;

	return *this;
}

template<typename T>
template<typename U>
constexpr U& ecs::pool<T>::at(std::size_t pos) {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);
	
	if (pos >= extent) {
		throw std::out_of_range("pool::at() - index out of range");
	}
	
	return data<U>()[pos / page_size][pos % page_size];
}

template<typename T>
template<typename U> 
constexpr const U& ecs::pool<T>::at(std::size_t pos) const {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);

	if (pos >= extent) {
		throw std::out_of_range("pool::at() - index out of range");
	}
	
	return data<U>()[pos / page_size][pos % page_size];
}

template<typename T>
template<typename U>
constexpr U& ecs::pool<T>::back() {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);

	if (extent == 0) {
		throw std::out_of_range("pool::back() - index out of range");
	}
	
	index_t pos = extent - 1;
	return data<U>()[pos / page_size][pos % page_size];
}

template<typename T>
template<typename U>
constexpr const U& ecs::pool<T>::back() const {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);

	if (extent == 0) {
		throw std::out_of_range("pool::back() - index out of range");
	}
	
	index_t pos = extent - 1;

	return data<U>()[pos / page_size][pos % page_size];
}

template<typename T>
template<typename U>
constexpr U& ecs::pool<T>::front() {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);

	if (extent == 0) {
		throw std::out_of_range("pool::front() - index out of range");
	}

	return data<U>()[0][0];
}

template<typename T>
template<typename U>
constexpr const U& ecs::pool<T>::front() const {
	static_assert(!std::is_same_v<std::remove_const_t<U>, indirect>);

	if (extent == 0) {
		throw std::out_of_range("pool::front() - index out of range");
	}

	return data<U>()[0][0];
}

template<typename T> constexpr ecs::pool<T>::reference ecs::pool<T>::at(std::size_t pos) {
	return *iterator{ this, pos };
}
template<typename T> constexpr ecs::pool<T>::const_reference ecs::pool<T>::at(std::size_t pos) const {
	return *const_iterator{ this, pos };
}
template<typename T> constexpr ecs::pool<T>::reference ecs::pool<T>::back() {
	return *(end() - 1);
}
template<typename T> constexpr ecs::pool<T>::const_reference ecs::pool<T>::back() const {
	return *(end() - 1);
}
template<typename T> constexpr ecs::pool<T>::reference ecs::pool<T>::front() {
	return *begin();
}
template<typename T> constexpr ecs::pool<T>::const_reference ecs::pool<T>::front() const {
	return *begin();
}

template<typename T> 
constexpr void ecs::pool<T>::clear() {
	index_t page_i = extent / page_size;
	index_t elem_i = extent % page_size;
	
	for (std::size_t i = 0; i < page_i; ++i) {
		std::destroy(storage[i], storage[i] + page_size);
	}
	std::destroy(storage[page_i], storage[page_i] + elem_i);

	extent = 0;
}


template<typename T>
[[nodiscard]] constexpr bool ecs::pool<T>::contains(const entity& key) const {
	index_t ent_page_i = key.index / page_size;
	index_t ent_elem_i = key.index % page_size;

	if (ent_page_i >= sparse_capacity) {
		return false;
	}
	
	indirect* ind_page = indexer[ent_page_i];

	if (ind_page == nullptr) {
		return false;
	}

	indirect& ind = ind_page[ent_elem_i];

	if (ind.index >= extent || ind.version != key.version) {
		return false;
	}

	index_t pos_page_i = ind.index / page_size;
	index_t pos_elem_i = ind.index % page_size;

	if (pos_page_i >= packed_capacity) {
		return false;
	}

	entity*& ent_page = manager[pos_page_i];
	
	if (ent_page == nullptr) {
		return false;
	}
	
	entity& ent = ent_page[pos_elem_i];

	if (ent != key) {
		return false;
	}

	return true;

}

template<typename T>
template<typename ... Ts>
constexpr ecs::iter<Ts...> ecs::pool<T>::begin() noexcept {
	return { this, 0 };
}

template<typename T>
template<typename ... Ts>
constexpr ecs::iter<Ts...> ecs::pool<T>::end() noexcept {
	return { this, extent };
}

template<typename T>
template<typename ... Ts>
constexpr ecs::iter<const Ts...> ecs::pool<T>::begin() const noexcept {
	return { this, 0 };
}

template<typename T>
template<typename ... Ts>
constexpr ecs::iter<const Ts...> ecs::pool<T>::end() const noexcept {
	return { this, extent };
}


template<typename T>
template<typename ... Ts>
constexpr std::reverse_iterator<ecs::iter<Ts...>> ecs::pool<T>::rbegin() noexcept {
	return std::reverse_iterator<ecs::iter<Ts...>>{ end() };
}

template<typename T>
template<typename ... Ts>
constexpr std::reverse_iterator<ecs::iter<Ts...>> ecs::pool<T>::rend() noexcept {
	return std::reverse_iterator<ecs::iter<Ts...>>{ begin() };
}

template<typename T>
template<typename ... Ts>
constexpr std::reverse_iterator<ecs::iter<const Ts...>> ecs::pool<T>::rbegin() const noexcept {
	return { end() };
}

template<typename T>
template<typename ... Ts>
constexpr std::reverse_iterator<ecs::iter<const Ts...>> ecs::pool<T>::rend() const noexcept {
	return { begin() };
}

template<typename T> 
template<typename ... Ts>
constexpr ecs::iter<Ts...> ecs::pool<T>::find(const entity& key) {
	index_t ent_page_i = key.index / page_size;
	index_t ent_elem_i = key.index % page_size;
	
	if (ent_page_i >= sparse_capacity) {
		return end<Ts...>();
	}
	
	indirect* ind_page = indexer[ent_page_i];

	if (ind_page == nullptr) {
		return end<Ts...>();
	}

	indirect& ind = ind_page[ent_elem_i];

	if (ind.index >= extent || ind.version != key.version) {
		return end<Ts...>();
	}

	index_t pos_page_i = ind.index / page_size;
	index_t pos_elem_i = ind.index % page_size;

	if (pos_page_i >= packed_capacity) {
		return end<Ts...>();
	}

	entity*& ent_page = manager[pos_page_i];
	
	if (ent_page == nullptr) {
		return end<Ts...>();
	}
	
	entity& ent = ent_page[pos_elem_i];

	if (ent != key) {
		return end<Ts...>();
	}

	struct { 
		indirect* indirect_pointer;
		entity **entity_page_pointer, *entity_pointer;
	} hint{ &ind, &ent_page, &ent };

	return { this, ind.index, hint };
}

template<typename T>
template<typename ... Ts>
constexpr ecs::iter<const Ts...> ecs::pool<T>::find(const entity& key) const {
	index_t ent_page_i = key.index / page_size;
	index_t ent_elem_i = key.index % page_size;
	
	if (ent_page_i >= sparse_capacity) {
		return end<Ts...>();
	}
	
	const indirect* ind_page = indexer[ent_page_i];

	if (ind_page == nullptr) {
		return end<Ts...>();
	}

	const indirect& ind = ind_page[ent_elem_i];

	if (ind.index >= extent || ind.version != key.version) {
		return end<Ts...>();
	}

	index_t pos_page_i = ind.index / page_size;
	index_t pos_elem_i = ind.index % page_size;

	if (pos_page_i >= packed_capacity) {
		return end<Ts...>();
	}

	entity const* const& ent_page = manager[pos_page_i];
	
	if (ent_page == nullptr) {
		return end<Ts...>();
	}
	
	const entity& ent = ent_page[pos_elem_i];

	if (ent != key) {
		return end<Ts...>();
	}

	struct { 
		const indirect* indirect_pointer;
		const entity **entity_page_pointer, *entity_pointer;
	} hint{ &ind, &ent_page, &ent };

	return { this, ind.index, hint };
}


template<typename T>
template<typename ... arg_Ts>
constexpr T& ecs::pool<T>::emplace_back(const ecs::entity& ent, arg_Ts&& ... args) {
	reserve(extent + 1);
	
	indirect& ind = assure(ent);
	if (ind.index < extent && ind.version == ent.version) {
		index_t page_i = ind.index / page_size;
		index_t elem_i = ind.index % page_size;

		if (manager[page_i][elem_i] == ent) {
			return storage[page_i][elem_i];
		}
	}
	ind = { extent, ent.version };
	
	index_t page_i = extent / page_size;
	index_t elem_i = extent % page_size;
	
	++extent;
	
	manager[page_i][elem_i] = ent;
	return *std::construct_at(&storage[page_i][elem_i], std::forward<arg_Ts>(args)...);
}

template<typename T>
template<std::input_iterator It, typename ... arg_Ts>
constexpr void ecs::pool<T>::emplace_back(It first, It last, arg_Ts&& ... args) {
	reserve(extent + (last - first));
	
	for (auto it = first; it < last; ++it) {
		entity ent = *it;
		indirect& ind = assure(ent);

		if (ind.index < extent && ind.version == ent.version && manager[ind.index / page_size][ind.index % page_size] == ent) {
			continue;
		}

		ind = { extent, ent.version };

		index_t page_i = extent / page_size;
		index_t elem_i = extent % page_size;

		manager[page_i][elem_i] = ent;
		std::construct_at(&storage[page_i][elem_i], std::forward<arg_Ts>(args)...);

		++extent;
	}
}

template<typename T>
template<typename ... arg_Ts>
constexpr T& ecs::pool<T>::emplace(const_iterator pos, const entity& ent, arg_Ts&& ... args) {
	reserve(extent + 1);

	iterator it{ pos };
	if (it != end()) {
		std::move_backward(it, end(), end() + 1);
	}
	
	*it.entity_elem = ent;
	assure(ent) = { pos.idx, ent.version };
	T& val = *std::construct_at(it.component_elem, std::forward<arg_Ts>(args)...);
	
	++extent;
	return val;
}

/*
template<typename T>
template<typename ... Ts, typename ... arg_Ts>
constexpr T& ecs::pool<T>::emplace(iterator<T, Ts...> pos, const entity& ent, arg_Ts&& ... args) {
	reserve(extent + 1);
	
	iterator it{ this, pos.idx };
	if (it != end()) {
		*end() = std::move(*it);
	}
	
	*it.entity_elem = ent;
	assure(ent) = { pos.idx, ent.version };
	T& val = *std::construct_at(it.component_elem, std::forward<arg_Ts>(args)...);
	
	++extent;
	return val;
}
*/

template<typename T>
constexpr bool ecs::pool<T>::erase(const entity& ent) { 
	iterator it = find(ent);
	if (it == end()) throw "";
	
	erase(it);
	return true;
}

template<typename T>
template<std::input_iterator It, typename ... arg_Ts>
constexpr void ecs::pool<T>::emplace(const_iterator pos, It first, It last, arg_Ts&& ... args) {
	index_t count = last - first;
	reserve(extent + count);

	iterator it{ pos };
	iterator mv{ this, extent };

	if (it != end()) {
		std::move_backward(it, end(), end() + count);
	}

	index_t n = pos.idx;
	for (; first != last; ++first, ++it, ++n) {
		entity ent = *first;

		*it.entity_elem = ent;
		assure(ent) = { n, ent.version };
		std::construct_at(it.component_elem, std::forward<arg_Ts>(args)...);
	}

	extent += count;
}

/*
template<typename T>
template<std::input_iterator It, typename ... arg_Ts>
constexpr void ecs::pool<T>::emplace(policy::optimal, const_iterator pos, It first, It last, arg_Ts&& ... args) {
	index_t count = last - first;
	reserve(extent + count);

	iterator it{ this, pos.idx };
	iterator mv{ this, std::min(pos.idx + count, extent) };

	if (pos != end()) {
		std::move_backward(it, mv, end() + count);
	}
	
	index_t n = pos.idx;
	for (; first != last; ++first, ++it, ++n) {
		entity ent = *first;

		*it.entity_elem = ent;
		assure(ent) = { n, ent.version };
		std::construct_at(it.component_elem, std::forward<arg_Ts>(args)...);
	}

	extent += count;
}
*/

template<typename T>
constexpr void ecs::pool<T>::erase(const_iterator pos) {
	iterator it{ pos };
	iterator mv{ it + 1 };
	iterator nd = end();

	if (mv != nd) {
		std::move(mv, nd, it);
	} else {
		destroy(it);
	}

	--extent;
}

/*
template<typename T>
constexpr void ecs::pool<T>::erase(ecs::policy::optimal, const_iterator pos) {
	iterator it{ this, pos.idx };
	iterator mv{ this, extent - 1 };

	if (it != mv) {
		*it = std::move(*mv);
	}
	std::destroy(it);

	--extent;
}
*/

template<typename T>
constexpr void ecs::pool<T>::erase(const_iterator first, const_iterator last) {
	index_t count = last - first;

	iterator it{ first };
	iterator mv{ last };

	it = std::move(mv, end(), it);
	for (; it < last; ++it) {
		std::destroy_at(std::addressof(get<T>(*it)));
	}

	extent -= count;
}

/*
template<typename T>
constexpr void ecs::pool<T>::erase(ecs::policy::optimal, const_iterator first, const_iterator last) {
	index_t count = last - first;
	iterator it{ this, first.idx };
	iterator mv{ this, std::max(last.idx, extent - count) };

	it = std::move(mv, end(), it);
	for (; it < last; ++it) {
		std::destroy_at(&static_cast<T&>(*it));
	}
	
	extent -= count;
}
*/

template<typename T>
constexpr void ecs::pool<T>::pop_back() {
	if (extent == 0) {
		throw std::out_of_range("pool::pop_back() - index out of range");
	}

	std::destroy_at(&data<T>()[extent / page_size][extent % page_size]);
	--extent;
}

template<typename T>
constexpr void ecs::pool<T>::reserve(std::size_t n) {
	if (n < capacity()) return;
	
	// allocates +1 elements for 1 past the end iterator
	// ++n;

	// guarantees past the end pointer to allocated block
	index_t new_page_extent = (n / page_size) + 1;
	
	// grow packed page allocation
	if (packed_capacity < new_page_extent) {
		index_t new_packed_capacity = std::bit_ceil(new_page_extent);

		// grow manager
		entity** new_manager = std::allocator<entity*>{}.allocate(new_packed_capacity);
		std::copy(manager, manager + page_extent, new_manager);
		std::fill(new_manager + new_page_extent, new_manager + new_packed_capacity, nullptr);
		std::allocator<entity*>{}.deallocate(manager, packed_capacity);
		manager = new_manager;

		// grow storage
		T**      new_storage = std::allocator<T*>{}.allocate(new_packed_capacity);
		std::copy(storage, storage + page_extent, new_storage);
		std::fill(new_storage + new_page_extent, new_storage + new_packed_capacity, nullptr);
		std::allocator<T*>{}.deallocate(storage, packed_capacity);
		storage = new_storage;
		
		packed_capacity = new_packed_capacity;
	}
	
	// grow packed elem allocation
	for (std::size_t i = page_extent; i < new_page_extent; ++i) {
		manager[i] = std::allocator<entity>{}.allocate(page_size);
		storage[i] = std::allocator<T>{}.allocate(page_size);
	}

	page_extent = new_page_extent;
}

template<typename T>
constexpr void ecs::pool<T>::shrink_to_fit() {
	// guarantees past the end pointer to allocated block
	index_t new_page_extent = (extent / page_size) + 1;

	// shrink packed elem allocations
	for (std::size_t i = new_page_extent; i < page_extent; ++i) {
		std::allocator<entity>{}.deallocate(manager[i], page_size);
		std::allocator<T>{}.deallocate(storage[i], page_size);
	}

	// shrink packed page allocations
	if (new_page_extent < packed_capacity) {
		index_t new_packed_capacity = std::bit_ceil(new_page_extent);

		// shrink manager
		entity** new_manager = std::allocator<entity*>{}.allocate(new_packed_capacity);
		std::copy(manager, manager + new_page_extent, new_manager);
		std::fill(new_manager + new_page_extent, new_manager + new_packed_capacity, nullptr);
		std::allocator<entity*>{}.deallocate(manager, packed_capacity);
		manager = new_manager;
		
		// shrink storage
		T** new_storage = std::allocator<T*>{}.allocate(new_packed_capacity);
		std::copy(storage, storage + new_page_extent, new_storage);
		std::fill(new_storage + new_page_extent, new_storage + new_packed_capacity, nullptr);
		std::allocator<T*>{}.deallocate(storage, packed_capacity);
		storage = new_storage;

		packed_capacity = new_packed_capacity;
	}

	page_extent = new_page_extent;
}


template<typename T> 
template<typename U>
constexpr U** ecs::pool<T>::data() {
	if constexpr (std::is_same_v<U, entity>) {
		return manager;
	} else if constexpr (std::is_same_v<U, T>) {
		return storage;
	} else if constexpr (std::is_same_v<U, indirect>) {
		return indexer;
	}
}

template<typename T>
template<typename U>
constexpr U const* const* ecs::pool<T>::data() const {
	if constexpr (std::is_same_v<U, entity>) {
		return manager;
	} else if constexpr (std::is_same_v<U, T>) {
		return storage;
	} else if constexpr (std::is_same_v<U, indirect>) {
		return indexer;
	}
}

template<typename T>
constexpr ecs::indirect& ecs::pool<T>::assure(const ecs::entity& ent) {
	index_t ent_page_i = ent.index / page_size;
	index_t ent_elem_i = ent.index % page_size;
		
	if (ent_page_i >= sparse_capacity) {
		index_t new_sparse_capacity = std::bit_ceil(ent_page_i);
		indirect** new_indexer = std::allocator<indirect*>{}.allocate(new_sparse_capacity);

		std::copy(indexer, indexer + sparse_capacity, new_indexer);
		std::fill(new_indexer + sparse_capacity, new_indexer + new_sparse_capacity, nullptr);

		std::allocator<indirect*>{}.deallocate(indexer, sparse_capacity);
		
		sparse_capacity = new_sparse_capacity;
		indexer = new_indexer;
	}
	
	indirect*& page = indexer[ent_page_i]; 

	if (indexer[ent_page_i] == nullptr) {

		page = std::allocator<indirect>{}.allocate(page_size);
		std::fill(page, page + page_size, indirect{ });
	}
	
	return page[ent_elem_i];
}