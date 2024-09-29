#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <concepts>
#include <span>

// TODO: resize change size_t for page and element indexing
// TODO: add constexpr and noexcept to functions

namespace util {
	template<typename T, typename Alloc_T = std::allocator<T>, typename Page_Alloc_T=std::allocator<std::span<T, 4096>>>
	class paged_vector;
	
	template<typename T, typename Alloc_T = std::allocator<T>, typename Page_Alloc_T=std::allocator<std::span<T, 4096>>> 
	class paged_vector_iterator;

	template<typename T, typename Alloc_T, typename Page_Alloc_T>
	class paged_vector 
	{
	public:
		using allocator_type = Alloc_T;
		using page_allocator_type = Page_Alloc_T;
	
		using value_type = allocator_type::value_type;
		using page_type = page_allocator_type::value_type;
		using reference = T&;
		using const_reference = const reference;
		using pointer = T*;
		using const_pointer = const pointer;
		using iterator = paged_vector_iterator<T, allocator_type, page_allocator_type>;
		using const_iterator = paged_vector_iterator<const T, allocator_type, page_allocator_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		static constexpr size_t page_size = page_type::extent;
	public:
		constexpr paged_vector() noexcept(noexcept(allocator_type()) && noexcept(page_allocator_type())) : paged_vector(allocator_type(), page_allocator_type()) { }
		constexpr explicit paged_vector(const allocator_type& alloc) noexcept(noexcept(page_allocator_type())) : paged_vector(alloc, page_allocator_type()) { }		
		constexpr explicit paged_vector(const allocator_type& elem_alloc, const page_allocator_type& page_alloc) noexcept;
		constexpr explicit paged_vector(size_t n, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		constexpr paged_vector(size_t n, const T& value, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		template<std::input_iterator It>
		constexpr paged_vector(It first, It last, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		constexpr paged_vector(const paged_vector& other);
		constexpr paged_vector(paged_vector&& other);
		constexpr paged_vector(std::initializer_list<T> ilist, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		
		constexpr ~paged_vector();

		constexpr paged_vector& operator=(const paged_vector& other);
		constexpr paged_vector& operator=(paged_vector&& other);
		constexpr paged_vector& operator=(std::initializer_list<T> ilist);
		
		template<std::input_iterator It>
		void assign(It first, It last);
		void assign(size_t n, const T& value);
		void assign(std::initializer_list<T> ilist);
		
		constexpr allocator_type get_allocator() const noexcept;
		constexpr page_allocator_type get_page_allocator() const noexcept;

		// iterators
		constexpr iterator begin() noexcept;
		constexpr const_iterator begin() const noexcept;
		constexpr iterator end() noexcept;
		constexpr const_iterator end() const noexcept;
		constexpr reverse_iterator rbegin() noexcept;
		constexpr const_reverse_iterator rbegin() const noexcept;
		constexpr reverse_iterator rend() noexcept;
		constexpr const_reverse_iterator rend() const noexcept;
		
		constexpr const_iterator cbegin() const noexcept;
		constexpr const_iterator cend() const noexcept;
		constexpr const_reverse_iterator crbegin() const noexcept;
		constexpr const_reverse_iterator crend() const noexcept;
		
		// capacity
		[[nodiscard]] constexpr bool empty() const noexcept;
		constexpr size_t size() const;
		constexpr size_t capacity() const;
		constexpr void resize(size_t n);
		constexpr void resize(size_t n, const T& value);
		constexpr void reserve(size_t n);
		constexpr void shrink_to_fit();

		// element/page/data access
		constexpr reference operator[](size_t index);
		constexpr const_reference operator[](size_t index) const;
		constexpr reference at(size_t pos);
		constexpr const_reference at(size_t pos) const;
		constexpr reference front();
		constexpr const_reference front() const;
		constexpr reference back();
		constexpr const_reference back() const;
		constexpr page_type& get_page(size_t pos);
		constexpr const page_type& get_page(size_t pos) const;
		constexpr page_type* data() noexcept;
		constexpr const page_type* data() const noexcept;

		// modifiers
		template<typename ... Arg_Ts>
		constexpr reference emplace_back(Arg_Ts&&... args);
		constexpr void push_back(const T& value);
		constexpr void push_back(T&& value);
		constexpr void pop_back();
		template<typename ... Arg_Ts>
		constexpr iterator emplace(const_iterator pos, Arg_Ts&&... args);
		constexpr iterator insert(const_iterator pos, const T& value);
		constexpr iterator insert(const_iterator pos, T&& value);
		constexpr iterator insert(const_iterator pos, size_t n, const T& value);
		template<std::input_iterator It>
		constexpr iterator insert(const_iterator pos, It first, It last);
		constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist);
		constexpr iterator erase(const_iterator pos, size_t n=1);
		constexpr iterator erase(const_iterator first, const_iterator last);
		constexpr void swap(paged_vector& other);
		constexpr void clear() noexcept;
	private:
		size_t extent;
		allocator_type alloc;
		std::vector<page_type, page_allocator_type> pages;
	};

	template<class It, typename Alloc_T = std::allocator<typename It::value_type>, typename Page_Alloc_T=std::allocator<std::span<typename It::value_type, 4096>>>
	paged_vector(It, It, Alloc_T = Alloc_T(), Page_Alloc_T = Page_Alloc_T())
	 -> paged_vector<typename It::value_type, Alloc_T, Page_Alloc_T>;

	template<typename T, typename Alloc_T, typename Page_Alloc_T>
	class paged_vector_iterator 
	{
	public:
		using allocator_type = Alloc_T;
		using page_allocator_type = Page_Alloc_T;
		
		using iterator_category = std::random_access_iterator_tag;
		using value_type = allocator_type::value_type;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;
		using const_pointer = const pointer;
		using const_reference = const reference;
		
		using page_type = page_allocator_type::value_type;
		static constexpr size_t page_size = page_type::extent;
		
		using container_type = std::conditional_t<std::is_const_v<T>, 
			const std::vector<page_type, page_allocator_type>, 
			std::vector<page_type, page_allocator_type>>;
	public:
		constexpr paged_vector_iterator();
		constexpr paged_vector_iterator(container_type* base, size_t index);
		constexpr paged_vector_iterator(container_type* base, size_t page_index, size_t elem_index);
		
		constexpr operator paged_vector_iterator<const T, Alloc_T, Page_Alloc_T>() const;
		
		constexpr reference operator*();
		constexpr const_reference operator*() const;

		constexpr reference operator[](difference_type n);
		constexpr const_reference operator[](difference_type n) const;
		
		constexpr bool operator==(const paged_vector_iterator& other) const;
		constexpr bool operator!=(const paged_vector_iterator& other) const;
		constexpr bool operator<(const paged_vector_iterator& other) const;
		constexpr bool operator<=(const paged_vector_iterator& other) const;
		constexpr bool operator>(const paged_vector_iterator& other) const;
		constexpr bool operator>=(const paged_vector_iterator& other) const;
		
		constexpr paged_vector_iterator& operator++(); // pre increment
		constexpr paged_vector_iterator operator++(int); // post increment
		constexpr paged_vector_iterator& operator--(); // pre increment
		constexpr paged_vector_iterator operator--(int); // post increment
		constexpr paged_vector_iterator operator+(difference_type n) const; 
		constexpr paged_vector_iterator operator-(difference_type n) const;
		constexpr paged_vector_iterator& operator+=(difference_type n);
		constexpr paged_vector_iterator& operator-=(difference_type n);

		constexpr difference_type operator-(const paged_vector_iterator& other) const;

		constexpr size_t index() const;

	public:
		size_t page_index;
		size_t elem_index;
	private:
		container_type* base;
	};

	template<typename T, typename Alloc_T, typename Page_Alloc_T> 
	paged_vector_iterator<T, Alloc_T, Page_Alloc_T> operator+(
		typename paged_vector_iterator<T, Alloc_T, Page_Alloc_T>::difference_type n, 
		const paged_vector_iterator<T, Alloc_T, Page_Alloc_T>& it);
}

#include "paged_vector.tpp"

static_assert(std::ranges::random_access_range<util::paged_vector<int>>);
static_assert(std::random_access_iterator<util::paged_vector<int>::iterator>);