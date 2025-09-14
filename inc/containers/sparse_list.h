

#pragma once
#include <memory>
#include <vector>
#include <array>
#include <cassert>
/*
sparse list set. a linked list with sparse index lookup determining 
if the value is inside the list. when pushing an index, indices within 
a page bound are grouped together to improve cache coherency when 
iterating. only supports clear, and pop_back operations but does 
support individual paged clear and pop_back also.
*/

namespace ecs {
	template<std::size_t N, bool Signed = true> struct integer {
		using type = std::conditional_t<Signed,
			std::conditional_t<(N > 32), int64_t, 
			std::conditional_t<(N > 16), int32_t, 
			std::conditional_t<(N > 8) , int16_t, int8_t>>>,
			std::conditional_t<(N > 32), uint64_t, 
			std::conditional_t<(N > 16), uint32_t, 
			std::conditional_t<(N > 8) , uint16_t, int8_t>>>>;
	};
	template<std::size_t N>
	using integer_t = typename integer<N, true>::type;

	template<std::size_t N>
	using unsigned_integer_t = typename integer<N, false>::type;
}

namespace ecs {
	template<std::unsigned_integral Key_T, std::size_t N=4096>
	class sparse_list {
	private:
		using key_type = Key_T;
		static constexpr std::size_t page_size = N;
		using elem_index_type = unsigned_integer_t<page_size>;
		using page_index_type = unsigned_integer_t<sizeof(key_type) * 8 / N>;
				
		struct page {
			std::unique_ptr<std::array<elem_index_type, page_size>> data;
			elem_index_type head = end_elem; // head of update list within page
			page_index_type next = null_page; // next page index
			page_index_type prev = null_page; // prev page index
		};

		static constexpr page_index_type null_page = -2;
		static constexpr page_index_type end_page = -1;
		
		static constexpr elem_index_type null_elem = -2;
		static constexpr elem_index_type end_elem = -1;
		
		// N must be able to represent 0-N as well as the reserved null and end values. 
		static_assert(N <= static_cast<elem_index_type>(-2));
			
	public:
		struct sentinel { };

		struct iterator {
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::size_t;

			iterator() = default;
			iterator(const sparse_list* base) : base(base) { 
				if (base != nullptr && base->head != end_elem) {
					page_i = base->head;
					elem_i = base->data[page_i].head;
				} else {
					base = nullptr;
				}
			}

			std::size_t operator*() const {
				return page_i * N + elem_i;
			}
			
			iterator& operator++() { 
				const auto& curr_page = base->data[page_i];
				const auto& curr_elem = curr_page[elem_i];

				if (curr_elem != end_elem) { // if not at end of elem list
					elem_i = curr_elem; // iterate to next element within page 
				} 
				else if (curr_page.prev != end_page) { // if not at end of page list
					page_i = curr_page.prev; // iterate to next page
					elem_i = base->data[page_i].head;
				} 
				else { // flag as end of list
					base = nullptr;
				}

				return *this;
			}
			
			iterator operator++(int) { 
				auto tmp = *this; ++(*this); return tmp; 
			}

			friend bool operator==(const iterator& lhs, const iterator& rhs) {
				return lhs.base == rhs.base && lhs.page_i == rhs.page_i && lhs.elem_i == rhs.elem_i; 
			}

			friend bool operator==(const iterator& lhs, sentinel) { 
				return lhs.base == nullptr;
			}

			const sparse_list* base;
			page_index_type page_i;
			elem_index_type elem_i;
		};

		struct page_view {
			struct sentinel { };
			/*
			concepts(264, 10): Because 'assignable_from<ecs::sparse_list<unsigned int, 4096>::page_view::iterator &, ecs::sparse_list<unsigned int, 4096>::page_view::iterator>' evaluated to false
			concepts(143, 10): Because '__lhs = static_cast<_Rhs &&>(__rhs)' would be invalid: object of type 
			'ecs::sparse_list<unsigned int, 4096>::page_view::iterator' cannot be assigned because its copy assignment operator is implicitly deleted
			*/
			struct iterator {
				using iterator_category = std::forward_iterator_tag;
				using difference_type = std::ptrdiff_t;
				using value_type = std::size_t;

				iterator() = default;
				iterator(page* ptr, page_index_type page_index) { 
					if (ptr == nullptr || ptr->head == end_elem) {
						base = nullptr;
					} else {
						base = ptr;
						page_i = page_index;
						elem_i = base->head;
					}
				}
				
				std::size_t operator*() const { 
					return page_i * N + elem_i; 
				}	
				
				iterator& operator++() { 
					const auto& curr_elem = (*base)[elem_i];

					if (curr_elem != end_elem) { 
						elem_i = curr_elem;
					} else {
						base = nullptr;
					}

					return *this;
				}
				
				iterator operator++(int) { 
					auto tmp = *this; 
					++(*this); 
					return tmp;
				}

				friend bool operator==(const iterator& lhs, const iterator& rhs) { 
					return lhs.base == rhs.base && lhs.elem_i == rhs.elem_i; 
				}
				
				friend bool operator==(const iterator& lhs, sentinel) { 
					return lhs.base == nullptr; 
				}

			private:
				const page* base;
				page_index_type page_i;
				elem_index_type elem_i;
			};

			page_view(sparse_list& base, page_index_type page_i) : base(base), page_i(page_i) { 
				base.reserve_page(page_i);
			}
			page_view(page_view&&) = default;
			page_view& operator=(page_view&&) = default;
			page_view(const page_view&) = delete;
			page_view& operator=(const page_view&) = delete;

			iterator begin() const { 
				return { &base.data[page_i], page_i };
			}
			sentinel end() const { 
				return { }; 
			}

            bool contains(std::size_t idx) const { 
				auto& page = base.data[page_i];
				return page.data != nullptr && (*page.data)[idx % N] != null_elem;
			}

			void clear() {
				auto& page = base.data[page_i];
				base.pop_page(page_i);
			}

			void push_back(key_type key) {
				assert(key / page_size == page_i);
				elem_index_type elem_i = key % page_size;

				base.alloc_page(page_i);
				
				base.push_page(page_i);
				
				auto& page = base.data[page_i];
				elem_index_type& curr = (*page.data)[elem_i];

				if (curr != null_elem) 
					return;

				curr = page.head;
				page.head = elem_i;
			}

			void pop_back() {
				auto& page = base.data[page_i];
				
				auto next = (*page.data)[page.head];
				page.head = next;
				
				if (page.head == end_elem) {
					base.pop_page();
				}
			}
			
			std::size_t back() const {
				return page_i * N + base.data[page_i].head;
			}

			bool empty() const {
				return base.data[page_i].head == end_elem;
			}
		private:
			sparse_list& base;
			const page_index_type page_i;
		};

		bool contains(std::size_t idx) const {
			page_index_type page_i = idx / N;
			elem_index_type elem_i = idx % N;
			if (page_i >= data.size()) return false;
			return get_page(page_i).contains(elem_i);
		}

		void push_back(std::size_t idx) {
			page_index_type page_i = idx / N;
			elem_index_type elem_i = idx % N;
			reserve_page(page_i);
			get_page().push_back(elem_i);
		}

		void push_n(std::size_t begin, std::size_t n) {
			
			page_index_type page_i = begin / N;
			elem_index_type elem_i = begin % N;

			std::size_t end = (begin + n);
			page_index_type page_end_i = end / N;
			elem_index_type elem_end_i = end % N;

			reserve_page(page_end_i);

			for (; page_i != page_end_i; ++page_i, elem_i = 0) {
				push_page(page_i);

				alloc_page(page_i);

				for (; elem_i != N; ++elem_i) {
					push_back(elem_i);
				}
			}

			if (elem_i != elem_end_i) {
				push_page(page_i);
				
				alloc_page(page_i);

				for (; elem_i != elem_end_i; ++elem_i) {
					push_back(elem_i);
				}
			}
		}

		void pop_back() {
			if (empty()) throw std::exception{};
			
			page_index_type page_i = head;
			auto& page = data[page_i];

			elem_index_type elem_i = page.head;
			
			if (page.head == end_elem) { // if empty pop back page
				head = page.prev;
				if (page.prev != end_page) data[page.prev].next = page.prev;
				
				page.data = nullptr;
				page.head = end_elem;
				page.prev = null_page;
				page.next = null_page;
			}
		}

		std::size_t back() const {
			page_index_type page_i = head;
			elem_index_type elem_i = data[head].head;
			std::size_t idx = page_i * N + elem_i;
			return idx;
		}

		void clear() {
			data.clear();
			head = end_page;
		}

		bool empty() const {
			return head == end_page;
		}

		iterator begin() const {
			return { this };
		}

		sentinel end() const {
			return { };
		}

		page_view get_page(std::size_t page_i) {
			return { *this, static_cast<page_index_type>(page_i) };
		}

		const page_view get_page(std::size_t page_i) const {
			return { *this, static_cast<page_index_type>(page_i) };
		}

		page_view page_back() {
			return { this, head };
		}

		const page_view page_back() const {
			return { this, head };
		}


	private:
		/* add page to update list */
		inline void push_page(page_index_type page_i) {
			auto& page = data[page_i];

			if (page.head == end_elem) {
				if (head != end_page) {
					data[head].next = page_i;
				}
				
				page.prev = head;
				page.next = end_page;

				head = page_i;
			}
		}

		/* pop page from update list */
		inline void pop_page(page_index_type page_i) {
			auto& page = data[page_i];

			if (head != page_i) {
				if (page.prev != end_page) data[page.prev].next = page.next; 
				if (page.next != end_page) data[page.next].prev = page.prev;
			} else {
				head = page.prev;
				if (page.prev != end_page) data[page.prev].next = end_page;
			}

			page.data = nullptr;
			page.head = end_elem;
			page.prev = null_page;
			page.next = null_page;
		}

		/* add page to update list */
		inline void alloc_page(page_index_type page_i) {
			auto& page = data[page_i];
			if (page.data == nullptr) {
				page.data = std::unique_ptr<std::array<elem_index_type, page_size>>(new std::array<elem_index_type, page_size>());
				std::fill_n(page.data->begin(), page_size, null_elem);
			}
		}
		/* reserve up to page index */
		inline void reserve_page(page_index_type page_i) {
			if (page_i < data.size()) data.resize(page_i + 1);
		}
		
		std::vector<page> data;
		page_index_type head = end_page;
	};
}

#include <cstdint>
#include <iterator>

static_assert(std::ranges::forward_range<ecs::sparse_list<uint32_t, 4096>>);
static_assert(std::forward_iterator<ecs::sparse_list<uint32_t, 4096>::iterator>);

static_assert(std::ranges::forward_range<ecs::sparse_list<uint32_t, 4096>::page_view>);
static_assert(std::forward_iterator<ecs::sparse_list<uint32_t, 4096>::page_view::iterator>);
