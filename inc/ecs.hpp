#pragma once
#include "ecs.h"
#include <functional>

// sparse map
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::pointer::operator->() const noexcept -> const reference*  { return std::addressof(ref); }

template<typename Key_T, typename Val_T>
constexpr ecs::sparse_map_iterator<Key_T, Val_T>::sparse_map_iterator(key_iterator k, mapped_iterator v) noexcept
 : key_it(k), mapped_it(v) { }

template<typename Key_T, typename Val_T>
constexpr ecs::sparse_map_iterator<Key_T, Val_T>::sparse_map_iterator(auto& map, size_type n) noexcept
 : key_it(map.keys().begin() + n), mapped_it(map.values().begin() + n) { }

template<typename Key_T, typename Val_T>
template<typename KC, typename VC> requires (std::is_const_v<Val_T> || !std::is_const_v<VC>)
constexpr ecs::sparse_map_iterator<Key_T, Val_T>::sparse_map_iterator(const KC& keys, VC& values, size_type pos) noexcept
 : key_it(keys.cbegin() + static_cast<difference_type>(pos)), mapped_it(values.begin() + static_cast<difference_type>(pos)) { }
	
template<typename Key_T, typename Val_T>
template<typename U> requires (std::is_const_v<Val_T> && !std::is_const_v<U> && std::is_same_v<Val_T, const U>)
constexpr ecs::sparse_map_iterator<Key_T, Val_T>::sparse_map_iterator(const sparse_map_iterator<Key_T, U>& other) noexcept
 : key_it(other.key_it), mapped_it(other.mapped_it) { }

template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator*() const noexcept -> reference {
	return reference(*key_it, *mapped_it);
}

template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator->() const noexcept -> pointer {
	return { **this };
}
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator[](difference_type n) const noexcept -> reference {
	return *(*this + n);
}

template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator++() noexcept -> sparse_map_iterator& {
	++key_it;
	++mapped_it;
	return *this;
}
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator++(int) noexcept -> sparse_map_iterator {
	auto tmp = *this;
	++*this;
	return tmp;
}
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator--() noexcept -> sparse_map_iterator& {
	--key_it;
	--mapped_it;
	return *this;
}
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator--(int) noexcept -> sparse_map_iterator {
	auto tmp = *this;
	--*this;
	return tmp;
}

template<typename Key_T, typename Val_T> 
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator+=(difference_type n) noexcept -> sparse_map_iterator& {
	key_it += n;
	mapped_it += n;
	return *this;
}

template<typename Key_T, typename Val_T> 
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator-=(difference_type n) noexcept -> sparse_map_iterator& {
	key_it -= n;
	mapped_it -= n;
	return *this;
}

template<typename Key_T, typename Val_T> 
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator+(difference_type n) const noexcept -> sparse_map_iterator {
	auto other = *this;
	return other += n;
}

template<typename Key_T, typename Val_T> 
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator-(difference_type n) const noexcept -> sparse_map_iterator {
	auto other = *this;
	return other -= n;
}

template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator-(const sparse_map_iterator& other) const noexcept -> difference_type {
	return key_it - other.key_it;
}
template<typename Key_T, typename Val_T>
constexpr auto ecs::sparse_map_iterator<Key_T, Val_T>::operator<=>(const sparse_map_iterator& other) const noexcept {
	return key_it <=> other.key_it;
}
template<typename Key_T, typename Val_T>
constexpr bool ecs::sparse_map_iterator<Key_T, Val_T>::operator==(const sparse_map_iterator& other) const noexcept {
	return key_it == other.key_it;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::find_pos(const key_type& key) const noexcept -> size_type {
	const size_type idx = key.index;
	const size_type page_index = idx >> page_shift;
	if (page_index >= sparse.size()) return key_type::max_index;
	
	const page_type& page = sparse[page_index];
	if (!page) return key_type::max_index;

	const size_type pos = sparse[page_index][idx & page_mask].index;
	return (pos < dense_keys.size() && dense_keys[pos] == key) ? pos : key_type::max_index;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::slot_for(const key_type& key) -> slot_type& {
	const size_type idx = key.index;
	const size_type page = idx >> page_shift;
	if (page >= sparse.size()) sparse.resize(page + 1);
	if (!sparse[page]) sparse[page] = std::make_unique<slot_type[]>(page_size);
	return sparse[page][idx & page_mask];
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::erase_at(size_type pos) {
	const size_type last = dense_keys.size() - 1;
	if (pos != last) {
		dense_keys[pos] = dense_keys[last];
		dense_values[pos] = std::move(dense_values[last]);
		slot_for(dense_keys[pos]).index = pos;
	}
	dense_keys.pop_back();
	dense_values.pop_back();
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::rebuild_sparse() {
	for (size_type i = 0; i < dense_keys.size(); ++i)
		slot_for(dense_keys[i]).index = i;
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(const sparse_map& other)
 : sparse(std::allocator_traits<page_allocator>::select_on_container_copy_construction(other.sparse.get_allocator())), dense_keys(other.dense_keys), dense_values(other.dense_values) {
	rebuild_sparse();
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(sparse_map&& other) noexcept
 : sparse(std::move(other.sparse)), dense_keys(std::move(other.dense_keys)), dense_values(std::move(other.dense_values)) {
	other.sparse.clear();
	other.dense_keys.clear();
	other.dense_values.clear();
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(const sparse_map& other, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(other.dense_keys, key_allocator(alloc)), dense_values(other.dense_values, mapped_allocator(alloc)) {
	rebuild_sparse();
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(sparse_map&& other, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(std::move(other.dense_keys), key_allocator(alloc)), dense_values(std::move(other.dense_values), mapped_allocator(alloc)) {
	if constexpr (std::allocator_traits<page_allocator>::is_always_equal::value)
		sparse = std::move(other.sparse);
	else
		rebuild_sparse();

	other.sparse.clear();
	other.dense_keys.clear();
	other.dense_values.clear();
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(key_container_type key_cont, mapped_container_type mapped_cont) {
	assert(key_cont.size() == mapped_cont.size());
	const size_type n = std::min(key_cont.size(), mapped_cont.size());
	dense_keys.reserve(n);
	dense_values.reserve(n);
	for (size_type i = 0; i < n; ++i)
		try_emplace(key_cont[i], std::move(mapped_cont[i]));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(const key_container_type& key_cont, const mapped_container_type& mapped_cont, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	assert(key_cont.size() == mapped_cont.size());
	const size_type n = std::min(key_cont.size(), mapped_cont.size());
	dense_keys.reserve(n);
	dense_values.reserve(n);
	for (size_type i = 0; i < n; ++i)
		try_emplace(key_cont[i], mapped_cont[i]);
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, key_container_type key_cont, mapped_container_type mapped_cont)
 : dense_keys(std::move(key_cont)), dense_values(std::move(mapped_cont)) {
	assert(dense_keys.size() == dense_values.size());
	rebuild_sparse();
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, const key_container_type& key_cont, const mapped_container_type& mapped_cont, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_cont, key_allocator(alloc)), dense_values(mapped_cont, mapped_allocator(alloc)) {
	assert(dense_keys.size() == dense_values.size());
	rebuild_sparse();
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) { }

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(It first, It last) {
	insert(first, last);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It, typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(It first, It last, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	insert(first, last);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename Range>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::from_range_t, Range&& rg) {
	insert_range(std::forward<Range>(rg));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename Range, typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::from_range_t, Range&& rg, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	insert_range(std::forward<Range>(rg));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, It first, It last) {
	insert(first, last);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It, typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, It first, It last, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	insert(first, last);
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::initializer_list<value_type> init) {
	insert(init);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::initializer_list<value_type> init, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	insert(init);
}

template<typename Key_T, typename Val_T, typename alloc_T>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, std::initializer_list<value_type> init) {
	insert(init);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename alloc_U>
ecs::sparse_map<Key_T, Val_T, alloc_T>::sparse_map(std::sorted_unique_t, std::initializer_list<value_type> init, const alloc_U& alloc)
 : sparse(page_allocator(alloc)), dense_keys(key_allocator(alloc)), dense_values(mapped_allocator(alloc)) {
	insert(init);
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::operator=(const sparse_map& other) -> sparse_map& {
	if (this != &other) {
		dense_keys = other.dense_keys;
		dense_values = other.dense_values;
		rebuild_sparse();
	}
	return *this;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::operator=(sparse_map&& other) -> sparse_map& {
	if (this != &other) {
		sparse = std::move(other.sparse);
		dense_keys = std::move(other.dense_keys);
		dense_values = std::move(other.dense_values);
		other.sparse.clear();
		other.dense_keys.clear();
		other.dense_values.clear();
	}
	return *this;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::operator=(std::initializer_list<value_type> ilist) -> sparse_map& {
	clear();
	insert(ilist);
	return *this;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::at(const key_type& key) -> mapped_type& {
	const size_type pos = find_pos(key);
	if (pos == key_type::max_index) throw std::out_of_range("sparse_map::at: key not found");
	return dense_values[pos];
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::at(const key_type& key) const -> const mapped_type&  {
	const size_type pos = find_pos(key);
	if (pos == key_type::max_index) throw std::out_of_range("sparse_map::at: key not found");
	return dense_values[pos];
}

template<typename Key_T, typename Val_T, typename alloc_T>
Val_T& ecs::sparse_map<Key_T, Val_T, alloc_T>::operator[](const key_type& key) {
	return try_emplace(key).first->second;
}

template<typename Key_T, typename Val_T, typename alloc_T>
Val_T& ecs::sparse_map<Key_T, Val_T, alloc_T>::operator[](key_type&& key) {
	return try_emplace(key).first->second;
}

template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::begin() noexcept -> iterator  { return iterator(dense_keys.cbegin(), dense_values.begin()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::begin() const noexcept -> const_iterator  { return const_iterator(dense_keys.cbegin(), dense_values.cbegin()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::cbegin() const noexcept -> const_iterator  { return begin(); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::end() noexcept -> iterator  { return iterator(dense_keys.cend(), dense_values.end()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::end() const noexcept -> const_iterator  { return const_iterator(dense_keys.cend(), dense_values.cend()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::cend() const noexcept -> const_iterator  { return end(); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::rbegin() noexcept -> reverse_iterator  { return reverse_iterator(end()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::rbegin() const noexcept -> const_reverse_iterator  { return const_reverse_iterator(end()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::crbegin() const noexcept -> const_reverse_iterator  { return rbegin(); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::rend() noexcept -> reverse_iterator  { return reverse_iterator(begin()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::rend() const noexcept -> const_reverse_iterator  { return const_reverse_iterator(begin()); }
template<typename Key_T, typename Val_T, typename alloc_T>
[[nodiscard]] auto ecs::sparse_map<Key_T, Val_T, alloc_T>::crend() const noexcept -> const_reverse_iterator  { return rend(); }

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::size() const noexcept -> size_type {
	return dense_keys.size();
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::empty() const noexcept -> bool {
	return dense_keys.empty();
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::max_size() const noexcept -> size_type {
	return std::min(dense_keys.max_size(), dense_values.max_size());
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename... arg_Ts>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::emplace(arg_Ts&&... args) -> std::pair<iterator, bool> {
	value_type v(std::forward<arg_Ts>(args)...);
	return try_emplace(v.first, std::move(v.second));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename... arg_Ts>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::try_emplace(key_type&& key, arg_Ts&&... args) -> std::pair<iterator, bool> {
	if (const size_type pos = find_pos(key); pos != key_type::max_index) return { begin() + pos, false };

	slot_type& slot = slot_for(key);
	const size_type pos = dense_keys.size();

	dense_values.emplace_back(std::forward<arg_Ts>(args)...);
	dense_keys.push_back(key);

	slot.index = pos;
	return { begin() + pos, true };
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename... arg_Ts>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::try_emplace(const key_type& key, arg_Ts&&... args) -> std::pair<iterator, bool> {

	if (const size_type pos = find_pos(key); pos != key_type::max_index) return { begin() + pos, false };

	slot_type& slot = slot_for(key);
	const size_type pos = dense_keys.size();

	dense_values.emplace_back(std::forward<arg_Ts>(args)...);
	dense_keys.push_back(key);

	slot.index = pos;
	slot.version = key.version;
	return { begin() + pos, true };
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(const value_type& value) -> std::pair<iterator, bool> {
	return try_emplace(value.first, value.second);
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(value_type&& value) -> std::pair<iterator, bool> {
	return try_emplace(value.first, std::move(value.second));
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(const_iterator, const value_type& value) -> iterator {
	return try_emplace(value.first, value.second).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(const_iterator, value_type&& value) -> iterator {
	return try_emplace(value.first, std::move(value.second)).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename P>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(P&& x) -> std::pair<iterator, bool> {
	return emplace(std::forward<P>(x));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename P>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(const_iterator, P&& x) -> iterator {
	return emplace(std::forward<P>(x)).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(It first, It last) {
	for (; first != last; ++first)
		emplace(*first);
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename It>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(std::sorted_unique_t, It first, It last) {
	insert(first, last);
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(std::initializer_list<value_type> ilist) {
	insert(ilist.begin(), ilist.end());
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::insert(std::sorted_unique_t, std::initializer_list<value_type> ilist) {
	insert(ilist.begin(), ilist.end());
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<typename Range>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_range(Range&& rg) {
	for (auto&& element : rg)
		emplace(std::forward<decltype(element)>(element));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(const key_type& k, M&& obj) -> std::pair<iterator, bool> {
	if (const size_type pos = find_pos(k); pos != key_type::max_index) {
		dense_values[pos] = std::forward<M>(obj);
		return { begin() + pos, false };
	}
	return try_emplace(k, std::forward<M>(obj));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(key_type&& k, M&& obj) -> std::pair<iterator, bool> {
	if (const size_type pos = find_pos(k); pos != key_type::max_index) {
		dense_values[pos] = std::forward<M>(obj);
		return { begin() + pos, false };
	}
	return try_emplace(k, std::forward<M>(obj));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class K, class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(K&& k, M&& obj) -> std::pair<iterator, bool> {
	const key_type key(std::forward<K>(k));
	if (const size_type pos = find_pos(key); pos != key_type::max_index) {
		dense_values[pos] = std::forward<M>(obj);
		return { begin() + pos, false };
	}
	return try_emplace(key, std::forward<M>(obj));
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(const_iterator, const key_type& k, M&& obj) -> iterator {
	return insert_or_assign(k, std::forward<M>(obj)).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(const_iterator, key_type&& k, M&& obj) -> iterator {
	return insert_or_assign(std::move(k), std::forward<M>(obj)).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class K, class M>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::insert_or_assign(const_iterator, K&& k, M&& obj) -> iterator {
	return insert_or_assign(std::forward<K>(k), std::forward<M>(obj)).first;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::erase(iterator position) -> iterator {
	const auto pos = static_cast<size_type>(position - begin());
	erase_at(pos);
	return begin() + pos;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::erase(const_iterator pos) -> iterator {
	const auto idx = static_cast<size_type>(pos - cbegin());
	erase_at(idx);
	return begin() + idx;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::erase(const_iterator first, const_iterator last) -> iterator {
	const auto f = static_cast<size_type>(first - cbegin());
	const auto l = static_cast<size_type>(last - cbegin());
	for (size_type i = l; i > f; --i)
		erase_at(i - 1);
	return begin() + f;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::erase(const key_type& key) -> size_type {
	const size_type pos = find_pos(key);
	if (pos == key_type::max_index) return 0;
	erase_at(pos);
	return 1;
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::swap(sparse_map& other) noexcept {
	sparse.swap(other.sparse);
	dense_keys.swap(other.dense_keys);
	dense_values.swap(other.dense_values);
}

template<typename Key_T, typename Val_T, typename alloc_T>
void ecs::sparse_map<Key_T, Val_T, alloc_T>::clear() noexcept {
	// Pages are kept; stale slots are harmless because lookups validate against dense_keys.
	dense_keys.clear();
	dense_values.clear();
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::find(const key_type& key) -> iterator {
	const size_type pos = find_pos(key);
	return pos == key_type::max_index ? end() : begin() + pos;
}

template<typename Key_T, typename Val_T, typename alloc_T>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::find(const key_type& key) const -> const_iterator {
	const size_type pos = find_pos(key);
	return pos == key_type::max_index ? end() : begin() + pos;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class K>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::find(const K& x) -> iterator {
	const key_type key(x);
	const size_type pos = find_pos(key);
	return pos == key_type::max_index ? end() : begin() + pos;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class K>
auto ecs::sparse_map<Key_T, Val_T, alloc_T>::find(const K& x) const -> const_iterator {
	const key_type key(x);
	const size_type pos = find_pos(key);
	return pos == key_type::max_index ? end() : begin() + pos;
}

template<typename Key_T, typename Val_T, typename alloc_T>
bool ecs::sparse_map<Key_T, Val_T, alloc_T>::contains(const key_type& key) const {
	return find_pos(key) != key_type::max_index;
}

template<typename Key_T, typename Val_T, typename alloc_T>
template<class K>
bool ecs::sparse_map<Key_T, Val_T, alloc_T>::contains(const K& x) const {
	const key_type key(x);
	return find_pos(key) != key_type::max_index;
}	

// manager
template<typename T>
[[nodiscard]] auto ecs::manager<T>::create() -> handle_type {
	if (free_list.empty()) {
		index_type index = static_cast<index_type>(entries.size());
		return entries.emplace_back(handle_type{ .index = index, .version = 0 });
	} else {
		index_type index = free_list.back();
		free_list.pop_back();

		entries.at(index).index = index;
		return { .index = index, .version = entries.at(index).version };
	}
}

template<typename T>
void ecs::manager<T>::destroy(handle_type hnd) noexcept {
	entries.at(hnd).index = handle_type::max_index;
	entries.at(hnd).version++;
	free_list.push_back(hnd.index);
}

template<typename T>
[[nodiscard]] bool ecs::manager<T>::alive(handle_type hnd) const noexcept {
	return hnd.index < entries.size() && entries.at(hnd.index) == hnd;
}

template<typename T>
void ecs::manager<T>::reset() noexcept {
	entries.clear();
	free_list.clear();
}

// registry
template<typename T, typename ... arg_Ts>
ecs::pool<T>& ecs::registry::get_pool(arg_Ts&&... args) {
	const std::size_t index = pool_index<T>();
	if (index >= pools.size()) {
		pools.resize(index + 1);
	}

	if (!pools[index]) {
		if constexpr (std::is_constructible_v<pool<T>, arg_Ts...>) {
			pools[index] = std::make_unique<pool<T>>(std::forward<arg_Ts>(args)...);
		} else {
			throw std::logic_error("");
		}
	}
	
	return *static_cast<ecs::pool<T>*>(pools[index].get());
}

template<typename T>
const ecs::pool<T>& ecs::registry::get_pool() const {
	const std::size_t index = pool_index<T>();
	if (index >= pools.size() || !pools[index]) {
		throw std::logic_error("registry::get_pool");
	}
	return *static_cast<ecs::pool<T>*>(pools[index].get());
}
template<typename T>
ecs::pool<T>* ecs::registry::try_pool() {
	const std::size_t index = pool_index<T>();
	if (index >= pools.size() || !pools[index]) {
		return nullptr;
	}
	return static_cast<ecs::pool<T>*>(pools[index].get());
}
template<typename T>
const ecs::pool<T>* ecs::registry::try_pool() const {
	const std::size_t index = pool_index<T>();
	if (index >= pools.size() || !pools[index]) {
		return nullptr;
	}
	return static_cast<ecs::pool<T>*>(pools[index].get());
}
template<typename T>
void ecs::registry::remove_pool() {
	const std::size_t index = pool_index<T>();
	if (index >= pools.size() || !pools[index]) {
		return;
	}
	pools[index] = nullptr;
}


// resource
template<ecs::traits::resource_class T>
[[nodiscard]] T& ecs::registry::get_resource() {
	return get_pool<T>();
}
template<ecs::traits::resource_class T>
[[nodiscard]] const T& ecs::registry::get_resource() const {
	return get_pool<T>();
}

template<ecs::traits::resource_class T>
[[nodiscard]] T* ecs::registry::try_resource() {
	return try_pool<T>();
}
template<ecs::traits::resource_class T>
[[nodiscard]] const T* ecs::registry::try_resource() const {
	return try_pool<T>();
}

template<ecs::traits::resource_class T, typename ... arg_Ts>
T& ecs::registry::add_resource(arg_Ts&&... args) {
	return pool<T>(std::forward<arg_Ts>(args)...);
}
template<ecs::traits::resource_class T> 
void ecs::registry::remove_resource() {
	remove_pool<T>();
}

// entity
template<ecs::traits::entity_class T, typename ... arg_Ts>
T ecs::registry::create(arg_Ts&&...args) {
	T ent = get_pool<T>().create(std::forward<arg_Ts>(args)...);
	emit(ecs::event::create<T>{ .entity = ent });
	return ent;
}
template<ecs::traits::entity_class T>
void ecs::registry::destroy(T ent) {
	emit(ecs::event::destroy<T>{ .entity = ent });
	get_pool<T>().destroy(ent);
}
template<ecs::traits::entity_class T>
bool ecs::registry::alive(T ent) const {
	const pool<T>* pl = try_pool<T>();
	return pl != nullptr && pl->alive(ent);
}

// event
template<ecs::traits::event_class T> [[nodiscard]] T& ecs::registry::get_listener(handle<T> hnd) {
	return get_pool<T>().at(hnd);
}
template<ecs::traits::event_class T> [[nodiscard]] const T& ecs::registry::get_listener(handle<T> hnd) const {
	return get_pool<T>().at(hnd);
}
template<ecs::traits::event_class T> [[nodiscard]] T* ecs::registry::try_listener(handle<T> hnd) {
	pool<T>* pl = try_pool<T>();
	if (!pl) return nullptr;

	auto it = pl->find(hnd);
	if (it == pl->end()) return nullptr;

	return std::addressof(it->second);
}
template<ecs::traits::event_class T> [[nodiscard]] const T* ecs::registry::try_listener(handle<T> hnd) const {
	const pool<T>* pl = try_pool<T>();
	if (!pl) return nullptr;

	auto it = pl->find(hnd);
	if (it == pl->end()) return nullptr;

	return std::addressof(it->second);
}
template<ecs::traits::event_class T>
void ecs::registry::emit(const T& event) {
	if (auto* pl = try_pool<T>(); pl != nullptr) {
		auto& mgr = static_cast<manager<handle<T>>&>(*pl);

		for (std::size_t i = 0; i < pl->size();) {
			const handle<T> hnd = pl->keys()[i];

			auto it = pl->find(hnd);
			if (it == pl->end() || it->second.callback == nullptr || !mgr.alive(hnd)) {
				++i;
				continue;
			}

			const bool once = it->second.once;
			it->second.callback(*this, event);

			if (once && mgr.alive(hnd) && pl->contains(hnd)) {
				pl->erase(hnd);
				mgr.destroy(hnd);
			} else {
				++i;
			}
		}
	}
}
template<typename T, typename func_T> [[nodiscard]] ecs::handle<T> ecs::registry::attach(void(*callback)(registry&, const T&), bool fire_once) {
	auto [it, valid] = get_pool<T>().insert({ callback, fire_once });
	if (!valid) throw std::logic_error("registry::attach");
}
template<typename T, auto func> [[nodiscard]] ecs::handle<T> ecs::registry::attach(bool fire_once) {
	return attach([](registry& reg, const T& event){ reg.invoke(func, event); }, fire_once);
}
template<ecs::traits::event_class T> bool ecs::registry::detach(handle<T> hnd) {
	return pool<T>().detach(hnd);
}


template<ecs::traits::component_class T> [[nodiscard]] T& ecs::registry::get_component(traits::get_entity_t<T> hnd) {
	return get_pool<T>().at(hnd);
}
template<ecs::traits::component_class T> [[nodiscard]] const T& ecs::registry::get_component(traits::get_entity_t<T> hnd) const {
	return get_pool<T>().at(hnd);
}

template<ecs::traits::component_class T> [[nodiscard]] T* ecs::registry::try_component(traits::get_entity_t<T> hnd) {
	ecs::pool<T>* pl = try_pool<T>();
	if (!pl) return nullptr;

	auto it = pl->find(hnd);
	if (it == pl->end()) return nullptr;

	return std::addressof(it->second);
}
template<ecs::traits::component_class T> [[nodiscard]] const T* ecs::registry::try_component(traits::get_entity_t<T> hnd) const {
	const ecs::pool<T>* pl = try_pool<T>();
	if (!pl) return nullptr;

	auto it = pl->find(hnd);
	if (it == pl->end()) return nullptr;

	return std::addressof(it->second);
}
template<ecs::traits::component_class T> [[nodiscard]] bool ecs::registry::has_component(traits::get_entity_t<T> e) const { 
	const pool<T>* pl = try_pool<T>();
	return pl != nullptr && pl->contains(e);
}
template<ecs::traits::component_class T, typename ... arg_Ts> T& ecs::registry::add_component(traits::get_entity_t<T> e, arg_Ts&& ... args) {
	auto& pl = get_pool<T>();
	
	auto [it, valid] = pl.try_emplace(e, std::forward<arg_Ts>(args)...);
	if (!valid) throw std::logic_error("registry::add_component");
	
	if constexpr (requires { event::add<T>{ .entity = e, .component = it->second }; }) {
		emit(event::add<T>{ .entity = e, .component = it->second });
	}
	return it->second;
}
template<ecs::traits::component_class T> void ecs::registry::remove_component(entity e) {
	auto& pl = get_pool<T>();

	auto it = pl.find(e);
	if (it == pl.end()) {
		throw std::logic_error("registry::remove_component");
	}
	
	if constexpr (requires { event::remove<T>{ .entity = e, .component = it->second }; }) {
		emit(event::remove<T>{ .entity = e, .component = it->second });
	}

	pl.erase(it);
}



namespace ecs::meta {
	template<typename ... Ts> struct type_list { };
	
	// at
	template<std::size_t I, typename L> struct at;
	template<std::size_t I, typename ... Ts> struct at<I, type_list<Ts...>> : std::tuple_element<I, std::tuple<Ts...>> { };
	template<std::size_t I, typename L> using at_t = typename at<I, L>::type;
	template<std::size_t I, typename L> inline constexpr std::size_t at_v = at<I, L>::value;

	// size
	template<typename L> struct size;
	template<typename ... Ts> struct size<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> { };
	template<typename L> inline constexpr std::size_t size_v = size<L>::value;

	// concat
	template<typename ... Ls> struct concat;
	template<> struct concat<> { using type = type_list<>; };
	template<typename ... Ts> struct concat<type_list<Ts...>> { using type = type_list<Ts...>; };
	template<typename ... Ts, typename ... Us, typename ... Vs> struct concat<type_list<Ts...>, type_list<Us...>, Vs...> : concat<type_list<Ts..., Us...>, Vs...> { };
	template<typename ... Ls> using concat_t = typename concat<Ls...>::type;
	
	// index of
	template<typename T, typename L> struct index_of;
	template<typename T, typename ... Ts> struct index_of<T, type_list<Ts...>> {
		static constexpr std::size_t value = []{
			std::size_t i = 0;
			if (((++i, std::is_same_v<T, Ts>) || ...)) return i - 1;
			else return i;
		}();
	};
	template<typename T, typename L> inline constexpr std::size_t index_of_v = index_of<T, L>::value;

	// func traits
	template<typename F> struct func_traits { };
	template<typename F> requires requires { &F::operator(); }
	struct func_traits<F> : func_traits<decltype(&F::operator())> { using base = void; };
	template<typename R, typename ... A> struct func_traits<R(A...)> {
		using return_type = R;
		using args = type_list<A...>;
		using base = void;
	};
	template<typename R, typename ... A> struct func_traits<R(A...) noexcept> : func_traits<R(A...)> { };
	template<typename R, typename ... A> struct func_traits<R(*)(A...)> : func_traits<R(A...)> { };
	template<typename R, typename ... A> struct func_traits<R(*)(A...) noexcept> : func_traits<R(A...)> { };
	template<typename R, typename ... A> struct func_traits<R(&)(A...)> : func_traits<R(A...)> { };
	template<typename R, typename ... A> struct func_traits<R(&)(A...) noexcept> : func_traits<R(A...)> { };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...)> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) noexcept> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...) const> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) const noexcept> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...) &> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) & noexcept> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...) const &> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) const & noexcept> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...) &&> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) && noexcept> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A> struct func_traits<R(B::*)(A...) const &&> : func_traits<R(A...)> { using base = B; };
	template<typename R, typename B, typename ... A>struct func_traits<R(B::*)(A...) const && noexcept> : func_traits<R(A...)> { using base = B; };
	
	template<typename T> struct is_func { static constexpr bool value = requires { typename func_traits<T>::args; }; };
	template<typename T> static constexpr bool is_func_v = is_func<T>::value;
	template<typename T> concept func_class = is_func<T>::value;
	namespace func {
		template<typename F> using get_return_t = typename func_traits<std::remove_cvref_t<F>>::return_type;
		template<typename F> using get_base_t = typename func_traits<std::remove_cvref_t<F>>::base;
		template<typename F> using get_args_t = typename func_traits<std::remove_cvref_t<F>>::args;
	}

	template<typename T>
	using bare_t = std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<T>>>;
 
	template<typename T>
	inline constexpr bool is_ptr_v = std::is_pointer_v<std::remove_cvref_t<T>>;
 
	template<typename T>
	using entity_of_t = traits::get_entity_t<bare_t<T>>;
 
	template<typename L> struct bare_list;
	template<typename... Ts> struct bare_list<type_list<Ts...>> { using type = type_list<bare_t<Ts>...>; };
	template<typename L> using bare_list_t = typename bare_list<L>::type;
 
	template<typename Pass, typename Dep>
	constexpr bool consumes() {
		using P = bare_t<Pass>;
		using D = bare_t<Dep>;
		if constexpr (std::is_same_v<P, D>)           return true;
		else if constexpr (traits::is_component_v<D>) return std::is_same_v<traits::get_entity_t<D>, P>;
		else                                          return false;
	}
 
	template<typename Pass, typename... Deps>
	constexpr bool is_consumed(type_list<Deps...>) { return (consumes<Pass, Deps>() || ...); }
 
	template<typename Dep, typename Passed> struct is_satisfied;
	template<typename Dep, typename... Ps>
	struct is_satisfied<Dep, type_list<Ps...>>
	 : std::bool_constant<!traits::is_component_v<bare_t<Dep>> || (consumes<Ps, Dep>() || ...)> { };
	template<typename Dep, typename Passed> inline constexpr bool is_satisfied_v = is_satisfied<Dep, Passed>::value;
 
	template<typename Deps, typename Passed> struct view_deps;
	template<typename... Ds, typename Passed> 
	struct view_deps<type_list<Ds...>, Passed>
	 : concat<std::conditional_t<is_satisfied_v<Ds, Passed>, type_list<>, type_list<Ds>>...> { };
	template<typename Deps, typename Passed> using view_deps_t = typename view_deps<Deps, Passed>::type;
 
	template<typename Views> inline constexpr bool same_entity_v = true;
	template<typename V, typename... Vs>
	inline constexpr bool same_entity_v<type_list<V, Vs...>> = (std::is_same_v<entity_of_t<V>, entity_of_t<Vs>> && ...);
 
	template<typename Views> struct select_primary { using type = void; };
	template<typename V, typename... Rest>
	struct select_primary<type_list<V, Rest...>> : std::conditional_t<is_ptr_v<V>, select_primary<type_list<Rest...>>, std::type_identity<bare_t<V>>> { };
	template<typename Views> using select_primary_t = typename select_primary<Views>::type;
 
	template<typename Views, typename primary_T> struct select_secondary;
	template<typename... Vs, typename primary_T>
	struct select_secondary<type_list<Vs...>, primary_T> : concat<std::conditional_t<std::is_same_v<bare_t<Vs>, primary_T>, type_list<>, type_list<Vs>>...> { };
	template<typename Views, typename primary_T> using select_secondary_t = typename select_secondary<Views, primary_T>::type;
 
	template<typename Views> struct ptr_tuple;
	template<typename... Vs> struct ptr_tuple<type_list<Vs...>> { using type = std::tuple<bare_t<Vs>*...>; };
	template<typename Views> using ptr_tuple_t = typename ptr_tuple<Views>::type;
 
}
 
 
namespace ecs::detail {
	template<typename T, std::size_t I, typename Local>
	decltype(auto) resolve_local(Local& local) {
		using slot_t = std::tuple_element_t<I, std::remove_cvref_t<Local>>;
		constexpr bool dep_ptr   = meta::is_ptr_v<T>;
		constexpr bool local_ptr = meta::is_ptr_v<slot_t>;
 
		if constexpr (dep_ptr == local_ptr) {
			return std::get<I>(local);
		} else if constexpr (dep_ptr) {
			return std::addressof(std::get<I>(local));
		} else {
			if (auto* ptr = std::get<I>(local); ptr != nullptr) return *ptr;
			throw std::logic_error("registry::invoke: null component for non-optional dependency");
		}
	}
 
	template<typename T>
	decltype(auto) resolve_registry(ecs::registry& reg) {
		if constexpr (meta::is_ptr_v<T>) return std::addressof(reg);
		else                             return (reg);
	}
 
	template<typename T>
	decltype(auto) resolve_resource(ecs::registry& reg) {
		using R = meta::bare_t<T>;
		if constexpr (meta::is_ptr_v<T>) return reg.try_resource<R>();
		else                             return reg.get_resource<R>();
	}
 
	template<typename T, std::size_t EntityIdx, typename Local>
	decltype(auto) resolve_component(ecs::registry& reg, Local& local) {
		using C = meta::bare_t<T>;
		auto& entity = std::get<EntityIdx>(local);
		if constexpr (meta::is_ptr_v<T>) return reg.try_component<C>(entity);
		else                             return reg.get_component<C>(entity);
	}
 
	template<typename T, typename... Ls>
	decltype(auto) resolve(ecs::registry& reg, std::tuple<Ls&...>& local) {
		using dependency_t = meta::bare_t<T>;
		using local_set_t  = meta::type_list<meta::bare_t<Ls>...>;
		
		if constexpr (meta::index_of_v<dependency_t, local_set_t> < meta::size_v<local_set_t>) {
			return resolve_local<T, meta::index_of_v<dependency_t, local_set_t>>(local);
		}
		else if constexpr (std::is_same_v<dependency_t, ecs::registry>) {
			return resolve_registry<T>(reg);
		}
		else if constexpr (traits::is_resource_v<dependency_t>) {
			return resolve_resource<T>(reg);
		}
		else {
			static_assert(traits::is_component_v<dependency_t>, "invoke: unsupported dependency type");
			constexpr std::size_t entity_idx = meta::index_of_v<traits::get_entity_t<dependency_t>, local_set_t>;
			
			static_assert(entity_idx < sizeof...(Ls), "invoke: no entity available for component dependency");
			return resolve_component<T, entity_idx>(reg, local);
		}
	}
 
	template<typename F, typename... Deps, typename Local>
	decltype(auto) call(ecs::registry& reg, F& func, meta::type_list<Deps...>, Local& local) {
		return std::invoke(func, resolve<Deps>(reg, local)...);
	}
 
	// every non-primary view has a pool
	template<typename... Views>
	bool has_pools(ecs::registry& reg, meta::type_list<Views...>) {
		return ((reg.try_pool<meta::bare_t<Views>>() != nullptr) && ...);
	}
 
	template<typename... Views, typename Ptrs, typename Entity>
	bool fetch(ecs::registry& reg, meta::type_list<Views...>, [[maybe_unused]] Ptrs& ptrs, [[maybe_unused]] const Entity& e) {
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return ((std::get<Is>(ptrs) = reg.try_component<meta::bare_t<Views>>(e), std::get<Is>(ptrs) != nullptr || meta::is_ptr_v<Views>) && ...);
		}(std::index_sequence_for<Views...>{});
	}
 
	template<typename Ptrs>
	auto tie_all(Ptrs& ptrs) {
		return std::apply([](auto&... elems) { return std::tie(elems...); }, ptrs);
	}
}

template<typename func_T, typename... pass_Ts>
decltype(auto) ecs::registry::invoke(func_T&& func, pass_Ts&&... args) {
	using deps_t   = meta::func::get_args_t<func_T>;
	using passed_t = meta::type_list<meta::bare_t<pass_Ts>...>;
	using views_t  = meta::view_deps_t<deps_t, passed_t>;
 
	static_assert((meta::is_consumed<pass_Ts>(deps_t{}) && ...), "not all passed args consumed");
 
	auto pass_args = std::tie(args...);
 
	if constexpr (meta::size_v<views_t> == 0) {
		return detail::call(*this, func, deps_t{}, pass_args);
	} else {
		static_assert(meta::same_entity_v<views_t>, "component dependencies belong to different entity types");
 
		using primary_t     = meta::select_primary_t<views_t>;
		static_assert(!std::is_void_v<primary_t>, "at least one component dependency must be non-optional");
 
		using secondary_t = meta::select_secondary_t<views_t, primary_t>;
		using ptrs_t        = meta::ptr_tuple_t<secondary_t>;
 
		auto* primary_pool = try_pool<primary_t>();
		if (!primary_pool || !detail::has_pools(*this, secondary_t{})) return;
 
		for (auto&& [entity, primary] : std::views::reverse(*primary_pool)) {
			ptrs_t ptrs{};
			if (!detail::fetch(*this, secondary_t{}, ptrs, entity)) continue;
 
			auto local = std::tuple_cat(pass_args, std::tie(entity, primary), detail::tie_all(ptrs));
			detail::call(*this, func, deps_t{}, local);
		}
	}
}