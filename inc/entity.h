#pragma once
#include "fwd.h"
#include <memory>



namespace ecs {
	template<>
	class pool<entity> final {
		static constexpr index_t null_index = ~(static_cast<index_t>(-1) << (ECS_INDEX_WIDTH));

		struct page_t { entity* data; index_t next; index_t head; };
	public:
		constexpr explicit pool() noexcept;
		constexpr ~pool() noexcept;
		constexpr pool(pool&& other) noexcept;
		constexpr pool& operator=(pool&& other) noexcept;
		pool(const pool&) = delete;
		pool& operator=(const pool&) = delete;

		[[nodiscard]] constexpr entity create();
		constexpr void destroy(const entity& ent);
		[[nodiscard]] constexpr bool alive(const entity& ent) const;
		[[nodiscard]] constexpr index_t capacity() const;
		
	private:
		entity** storage;
		page_t*  archive;
        index_t  extent;
        index_t  sparse_capacity;
		index_t  head;
	};
}

constexpr ecs::pool<ecs::entity>::pool() noexcept
 : archive(nullptr), extent(0), sparse_capacity(8), head(null_index) {
	archive = std::allocator<page_t>{}.allocate(sparse_capacity);
	std::fill_n(archive, sparse_capacity, page_t{ nullptr, 0, null_index });
}

constexpr ecs::pool<ecs::entity>::~pool() noexcept {
	if (archive != nullptr) {
		for (page_t* it = archive, *end = archive + (extent + page_size - 1) / page_size; it != end; ++it) {
			std::allocator<entity>{}.deallocate(it->data, page_size);
		}
		std::allocator<page_t>{}.deallocate(archive, sparse_capacity);
	}
}

constexpr ecs::pool<ecs::entity>::pool(pool&& other) noexcept
 : archive(other.archive), extent(other.extent), sparse_capacity(other.sparse_capacity), head(other.head) {
	other.archive = nullptr;
	other.extent = 0;
	other.sparse_capacity = 0;
	other.head = null_index;
}

constexpr ecs::pool<ecs::entity>& ecs::pool<ecs::entity>::operator=(pool&& other) noexcept {
	if (this == &other) return *this;
	
	if (archive != nullptr) {
		for (page_t* it = archive, *end = archive + (extent + page_size - 1) / page_size; it != end; ++it) {
			std::allocator<entity>{}.deallocate(it->data, page_size);
		}
		std::allocator<page_t>{}.deallocate(archive, sparse_capacity);
	}
	
	archive = other.archive;
	extent = other.extent;
	sparse_capacity = other.sparse_capacity;
	head = other.head;
	
	other.archive = nullptr;
	other.extent = 0;
	other.sparse_capacity = 0;
	other.head = null_index;
	
	return *this;
} 

[[nodiscard]] constexpr ecs::entity ecs::pool<ecs::entity>::create() { 
	if (head != null_index) {
		page_t& page = archive[head];
		entity& elem = page.data[page.head];
		
		index_t ind = head * page_size + page.head;
		page.head = elem.index;
		
		elem.index = ind;
		elem.version++;

		if (page.head == null_index) {
			head = page.next;
		}

		return { elem.index, elem.version };
	} 
	
	if (extent == capacity()) {
		index_t new_cap = std::bit_ceil(sparse_capacity + 1);
		
		page_t* new_archive = std::allocator<page_t>{}.allocate(new_cap);
		
		std::copy(archive, archive + sparse_capacity, new_archive);
		std::fill_n(archive, sparse_capacity, page_t{ nullptr, 0, null_index });
		
		std::allocator<page_t>{}.deallocate(archive, sparse_capacity); 

		sparse_capacity = new_cap;
		archive = new_archive;
	}
	
	page_t& page = archive[extent / page_size];
	
	if (page.data == nullptr) {
		page.data = std::allocator<entity>{}.allocate(page_size);
		std::fill_n(page.data, page_size, entity{});
		page.head = null_index;
	}
	
	entity& elem = page.data[extent % page_size];
	elem.index = extent++;

	return { elem.index, elem.version };
}

constexpr void ecs::pool<ecs::entity>::destroy(const ecs::entity& ent) {
	if(!alive(ent)) {
		throw std::out_of_range("entity not alive");
	}
	
	const index_t page_index = ent.index / page_size;
	const index_t elem_index = ent.index % page_size;

	page_t& page = archive[page_index];

	if (page.head == null_index) {
		page.next = head;
		head = page_index;
	}

	page.data[elem_index].index = page.head;
	page.head = elem_index;
}

[[nodiscard]] constexpr bool ecs::pool<ecs::entity>::alive(const entity& ent) const {
	return (ent.index < extent) && (archive[ent.index / page_size].data[ent.index % page_size] == ent);
}

[[nodiscard]] constexpr ecs::index_t ecs::pool<ecs::entity>::capacity() const {
	return page_size * sparse_capacity;
}