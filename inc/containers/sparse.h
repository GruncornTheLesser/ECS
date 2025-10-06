#pragma once
#include <memory>
#include <vector>
#include <span>

namespace ecs {
	template<std::unsigned_integral T, std::size_t N=4096>
	class sparse;

	template<std::unsigned_integral T>
	class sparse_iterator;
	
	template<std::unsigned_integral T, std::size_t N>
	class sparse { 
		static constexpr std::size_t page_size = N;
		using page_type = std::span<T, page_size>;
		static constexpr T tombstone = static_cast<T>(-1);
	public:
		using key_type = std::size_t;
		using mapped_type = T;
		using value_type = std::pair<const key_type, mapped_type>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using allocator_type = std::allocator<T>; 
		using page_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<page_type>;
		using iterator = sparse_iterator<T>;
		using const_iterator = sparse_iterator<const T>;
		using data_type = std::vector<page_type, page_allocator_type>;

	public:
		constexpr sparse() noexcept(noexcept(allocator_type()) && noexcept(page_allocator_type())) : sparse(allocator_type(), page_allocator_type()) { }
		constexpr explicit sparse(const allocator_type& alloc) noexcept(noexcept(page_allocator_type())) : sparse(alloc, page_allocator_type()) { }
		constexpr explicit sparse(const allocator_type& elem_alloc, const page_allocator_type& page_alloc) noexcept;
		
		constexpr sparse(const sparse& other);
		constexpr sparse(sparse&& other);
		
		constexpr ~sparse();

		constexpr sparse& operator=(const sparse& other);
		constexpr sparse& operator=(sparse&& other);
		
		
		constexpr allocator_type& get_allocator() noexcept;
		constexpr page_allocator_type& get_page_allocator() noexcept;

		[[nodiscard]] constexpr std::size_t size() const;

		constexpr T& at(key_type key);
		constexpr const T& at(key_type key) const;

		constexpr iterator end() noexcept;
		constexpr const_iterator end() const noexcept;
		constexpr const_iterator cend() const noexcept;

		constexpr void clear() noexcept;

		constexpr std::pair<iterator, bool> emplace(key_type key, mapped_type val);
		constexpr std::size_t erase(key_type key);

		[[nodiscard]] constexpr iterator find(key_type key);
		[[nodiscard]] constexpr const_iterator find(key_type key) const;
		
		[[nodiscard]] constexpr bool contains(key_type key) const;
	private:
		allocator_type alloc;
		data_type pages;
		std::size_t count;
	};

	template<std::unsigned_integral T>
	class sparse_iterator {
	public:
		using key_type = std::size_t;
		using mapped_type = T;
		using value_type = std::pair<const key_type, mapped_type>;
		using difference_type = std::ptrdiff_t;
		using pointer = std::pair<const key_type, mapped_type&>;
		using reference = std::pair<const key_type, mapped_type&>;

		using iterator_category = std::random_access_iterator_tag;

	public:
		constexpr sparse_iterator() : ptr(nullptr) { };
		constexpr sparse_iterator(std::size_t key, pointer ptr) : ptr(ptr) { };
		constexpr operator sparse_iterator<const T>() const { return { key, ptr }; }
		constexpr reference operator*() const { return { key, *ptr }; }
		constexpr reference operator->() const { return { key, *ptr }; }
		constexpr friend bool operator==(const sparse_iterator& lhs, const sparse_iterator& rhs) { return lhs.ptr == rhs.ptr; }
	private:
		const key_type key;
		pointer ptr;
	};
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::sparse(const allocator_type& elem_alloc, const page_allocator_type& page_alloc) noexcept 
 : alloc(elem_alloc), pages(page_alloc) { }
		

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::sparse(const sparse& other) 
 : alloc(other.alloc), pages(other.pages) { }

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::sparse(sparse&& other)
: alloc(std::move(other.alloc)), pages(std::move(other.pages)) { }

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::~sparse() {
	clear();
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>& ecs::sparse<T, N>::operator=(const sparse& other) {
	if (this == &other) return *this;

	alloc = other.alloc;
	pages = other.pages;
}


template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>& ecs::sparse<T, N>::operator=(sparse&& other) {
	if (this == &other) return *this;

	alloc = std::move(other.alloc);
	pages = std::move(other.pages);
}
		
template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::allocator_type& 
ecs::sparse<T, N>::get_allocator() noexcept {
	return alloc;
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::page_allocator_type& 
ecs::sparse<T, N>::get_page_allocator() noexcept {
	return pages.get_allocator();
}

template<std::unsigned_integral T, std::size_t N>
[[nodiscard]] constexpr std::size_t ecs::sparse<T, N>::size() const {
	return count;
}



template<std::unsigned_integral T, std::size_t N>
constexpr T& ecs::sparse<T, N>::at(key_type key) {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;
	return pages.at(page_i).at(elem_i);
}

template<std::unsigned_integral T, std::size_t N>
constexpr const T& ecs::sparse<T, N>::at(key_type key) const {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;
	return pages.at(page_i).at(elem_i);
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::iterator 
ecs::sparse<T, N>::end() noexcept {
	return { tombstone, nullptr };
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::const_iterator 
ecs::sparse<T, N>::end() const noexcept {
	return { tombstone, nullptr };
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::const_iterator 
ecs::sparse<T, N>::cend() const noexcept {
	return { tombstone, nullptr };
}

template<std::unsigned_integral T, std::size_t N>
void constexpr ecs::sparse<T, N>::clear() noexcept {
	for (auto& page : pages) {
		if (page.data() != nullptr) {
			get_allocator().deallocate(page.data(), page_size);
		}
	}
	pages.clear();
}

template<std::unsigned_integral T, std::size_t N>
constexpr std::pair<typename ecs::sparse<T, N>::iterator, bool> ecs::sparse<T, N>::emplace(key_type key, mapped_type val) {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;

	if (pages.size() >= page_i) {
		pages.resize(std::bit_ceil(page_i), page_type(static_cast<mapped_type*>(nullptr), page_size));
	}

	auto& page = pages.at(page_i);

	if (page.data() == nullptr) {
		page = page_type{ get_allocator().allocate(page_size), page_size };
	}

	auto& elem = page[elem_i];

	if (elem == tombstone) {
		++count;
		elem = val;
		return std::pair{ iterator{ key, &elem }, true };
	} else {
		elem = val;
		return std::pair{ iterator{ key, &elem }, false };
	}
}

template<std::unsigned_integral T, std::size_t N>
constexpr std::size_t ecs::sparse<T, N>::erase(key_type key) {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;

	if (pages.size() >= page_i) {
		return 0;
	}

	auto& page = pages.at(page_i);

	if (page.data() == nullptr) {
		return 0;
	}
	
	auto& elem = page[elem_i];

	if (elem == tombstone) {
		return 0;
	}
	
	--count;
	elem = tombstone;
	
	return 1;
}

template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::iterator 
ecs::sparse<T, N>::find(key_type key) {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;

	if (pages.size() >= page_i) {
		return { key, nullptr };
	}

	auto& page = pages.at(page_i);

	if (page.data() == nullptr) {
		return { key, nullptr };
	}

	auto& elem = page.at(elem_i);

	if (elem == tombstone) {
		return { key, nullptr };
	}

	return { key, &elem };
}


template<std::unsigned_integral T, std::size_t N>
constexpr ecs::sparse<T, N>::const_iterator 
ecs::sparse<T, N>::find(key_type key) const {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;

	if (pages.size() >= page_i) {
		return { key, nullptr };
	}

	auto& page = pages.at(page_i);

	if (page.data() == nullptr) {
		return { key, nullptr };
	}

	auto& elem = page.at(elem_i);
	
	if (elem == tombstone) {
		return { key, nullptr };
	}

	return { key, &elem };
}

template<std::unsigned_integral T, std::size_t N>
[[nodiscard]] constexpr bool ecs::sparse<T, N>::contains(key_type key) const {
	std::size_t page_i = key / page_size;
	std::size_t elem_i = key % page_size;
	
	return (pages.size() < page_i) && (pages[page_i][elem_i] != tombstone);
}
