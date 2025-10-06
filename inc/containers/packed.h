#pragma once
#include <span>
#include <memory>
#include <vector>
#include <algorithm>

namespace ecs {
	template<typename T, std::size_t N=4096, typename Alloc_T=std::allocator<T>>
	class packed;

	template<typename T, std::size_t N=4096, typename Alloc_T=std::allocator<T>>
	class packed_iterator;

	template<typename T, std::size_t N, typename Alloc_T>
	class packed {
		template<typename, std::size_t, typename> friend class packed_iterator;
	public:
		using page_type = std::span<T, N>;
		using allocator_type = Alloc_T; 
		using page_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<page_type>;
		static constexpr size_t page_size = page_type::extent;
		using value_type = page_type::value_type;

		using pointer = T*;
		using reference = T&;
		using const_pointer = const T*;
		using const_reference = const T&;
		using iterator = packed_iterator<T, N, allocator_type>;
		using const_iterator = packed_iterator<const T, N, allocator_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	public:
		constexpr packed() noexcept(noexcept(allocator_type()) && noexcept(page_allocator_type())) : packed(allocator_type(), page_allocator_type()) { }
		constexpr explicit packed(const allocator_type& alloc) noexcept(noexcept(page_allocator_type())) : packed(alloc, page_allocator_type()) { }
		constexpr explicit packed(const allocator_type& elem_alloc, const page_allocator_type& page_alloc) noexcept;
		constexpr explicit packed(size_t n, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		constexpr packed(size_t n, const T& value, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		template<std::input_iterator It>
		constexpr packed(It first, It last, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());
		constexpr packed(const packed& other);
		constexpr packed(packed&& other);
		constexpr packed(std::initializer_list<T> ilist, const allocator_type& elem_alloc=allocator_type(), const page_allocator_type& page_alloc=page_allocator_type());

		constexpr ~packed();

		constexpr packed& operator=(const packed& other);
		constexpr packed& operator=(packed&& other);
		constexpr packed& operator=(std::initializer_list<T> ilist);

		template<std::input_iterator It>
		constexpr void assign(It first, It last);
		constexpr void assign(size_t n, const T& value);
		constexpr void assign(std::initializer_list<T> ilist);

		constexpr allocator_type& get_allocator() noexcept;
		constexpr page_allocator_type& get_page_allocator() noexcept;

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
		constexpr size_t page_count() const;
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
		constexpr iterator erase(const_iterator pos);
		constexpr iterator erase(const_iterator first, const_iterator last);
		constexpr void swap(packed& other);
		constexpr void clear() noexcept;

	private:
		allocator_type alloc;
		size_t extent;
		std::vector<page_type, page_allocator_type> pages;
	};

	template<typename T, std::size_t N, typename Alloc_T>
	class packed_iterator {
	public:
		using page_type = std::span<T, N>;
		using allocator_type = Alloc_T;
		using page_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<page_type>;
		
		static constexpr size_t page_size = page_type::extent;
		using value_type = typename page_type::value_type;
		
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		using iterator_category = std::random_access_iterator_tag;

		using base_range = std::conditional_t<std::is_const_v<T>,
			const packed<std::remove_const_t<T>, N, allocator_type>,
			packed<T, N, allocator_type>>;
	public:
		constexpr packed_iterator();
		constexpr packed_iterator(base_range* base, size_t index);
		constexpr packed_iterator(base_range* base, std::size_t page_index, std::size_t elem_index);

		constexpr operator packed_iterator<const T, N, Alloc_T>() const;

		constexpr reference operator*() const;
		constexpr page_type& get_page() const;
		constexpr reference operator[](difference_type n) const;

		constexpr bool operator==(const packed_iterator& other) const;
		constexpr bool operator!=(const packed_iterator& other) const;
		constexpr bool operator<(const packed_iterator& other) const;
		constexpr bool operator<=(const packed_iterator& other) const;
		constexpr bool operator>(const packed_iterator& other) const;
		constexpr bool operator>=(const packed_iterator& other) const;

		constexpr packed_iterator& operator++(); // pre increment
		constexpr packed_iterator operator++(int); // post increment
		constexpr packed_iterator& operator--(); // pre increment
		constexpr packed_iterator operator--(int); // post increment
		constexpr packed_iterator operator+(difference_type n) const;
		constexpr packed_iterator operator-(difference_type n) const;
		constexpr packed_iterator& operator+=(difference_type n);
		constexpr packed_iterator& operator-=(difference_type n);

		constexpr difference_type operator-(const packed_iterator& other) const;

		constexpr size_t get_index() const;
		constexpr std::pair<std::size_t, std::size_t> get_indices() const;
	
	private:
		base_range* range;
		std::size_t page_index;
		std::size_t elem_index;
	};

	template<typename T, std::size_t N, typename Alloc_T>
	packed_iterator<T, N, Alloc_T> operator+(std::ptrdiff_t n, const packed_iterator<T, N, Alloc_T>& it);
}

// constructors
template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(const allocator_type& elem_alloc, const page_allocator_type& page_alloc) noexcept
 : extent(0), alloc(elem_alloc), pages(page_alloc) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(size_t n, const allocator_type& elem_alloc, const page_allocator_type& page_alloc)
 : extent(n), alloc(elem_alloc), pages(page_alloc)
{
	reserve(extent);

	for (int page_i = 0; page_i < extent / page_size; ++page_i)
		std::uninitialized_default_construct_n(pages[page_i].data(), page_size);
	std::uninitialized_default_construct_n(pages.back().data(), extent % page_size);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(size_t n, const T& value, const allocator_type& elem_alloc, const page_allocator_type& page_alloc)
 : extent(n), alloc(elem_alloc), pages(page_alloc)
{
	reserve(extent);

	for (int page_i = 0; page_i < extent / page_size; ++page_i)
		std::uninitialized_fill_n(pages[page_i].data(), page_size, value);
	std::uninitialized_fill_n(pages.back().data(), extent % page_size, value);
}

template<typename T, std::size_t N, typename Alloc_T> template<std::input_iterator It>
constexpr ecs::packed<T, N, Alloc_T>::packed(It first, It last, const allocator_type& elem_alloc, const page_allocator_type& page_alloc)
 : extent(last - first), pages(page_alloc)
{
	reserve(extent);

	for (int page_i = 0; page_i < extent / page_size; ++page_i, first += page_size)
		std::uninitialized_copy_n(first, page_size, pages[page_i].data());
	std::uninitialized_copy_n(first, extent % page_size, pages.back().data());
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(const packed& other) // copy constructor
 : extent(other.extent), pages(other.pages.get_allocator())
{
	reserve(extent);

	for (int page_i = 0; page_i < extent / page_size; ++page_i)
		std::uninitialized_copy_n(other.pages[page_i].data(), page_size, pages[page_i].data());
	std::uninitialized_copy_n(other.pages.back().data(), extent % page_size, pages.back().data());

}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(packed&& other) // move constructor
 : extent(other.extent), pages(std::move(other.pages)) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::packed(std::initializer_list<T> ilist, const allocator_type& elem_alloc, const page_allocator_type& page_alloc)
 : packed(ilist.begin(), ilist.end(), elem_alloc, page_alloc) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::~packed()
{
	if (pages.data() != nullptr)
	{
		size_t page_n = extent / page_size;
		size_t elem_n = extent % page_size;

		for (int page_i = 0; page_i < page_n; ++page_i)
		{
			std::destroy_n(pages[page_i].data(), page_size);
			std::destroy_at(&pages[page_i]);
		}

		std::destroy_n(pages[page_n].data(), elem_n);
		std::destroy_at(&pages[page_n]);

		// deallocate pages
		for (int page_i = 0; page_i < page_count(); ++page_i)
			get_allocator().deallocate(pages[page_i].data(), page_size);
	}
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>&
ecs::packed<T, N, Alloc_T>::operator=(const packed& other) // copy assignment
{
	assign(other.begin(), other.end());
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>&
ecs::packed<T, N, Alloc_T>::operator=(packed&& other) // move assignment
{
	if (this != &other)
	{
		std::swap(extent, other.extent);
		std::swap(pages, other.pages);
	}
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>&
ecs::packed<T, N, Alloc_T>::operator=(std::initializer_list<T> ilist)
{
	assign(ilist);
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
template<std::input_iterator It>
constexpr void ecs::packed<T, N, Alloc_T>::assign(It first, It last)
{
	size_t count = last - first;
	if (count == 0) return;

	reserve(count);

	size_t page_n = extent / page_size;
	size_t elem_n = extent % page_size;
	size_t other_page_n = count / page_size;
	size_t other_elem_n = count % page_size;

	if (extent < count)
	{
		for (int page_i = 0; page_i < page_n; ++page_i, first += page_size)
			std::copy_n(first, page_size, pages[page_i].data());
		std::copy_n(first, elem_n, pages[page_n].data());

		first += elem_n;

		if (page_n == other_page_n)
		{
			std::uninitialized_copy_n(first, other_elem_n - elem_n, pages[page_n].data() + elem_n);
		}
		else
		{
			std::uninitialized_copy_n(first, page_size - elem_n, pages[page_n].data() + elem_n);
			first += page_size;

			for (size_t page_i = page_n + 1; page_i < other_page_n; ++page_i, first += page_size)
				std::uninitialized_copy_n(first, page_size, pages[page_i].data());
			std::uninitialized_copy_n(first, other_elem_n, pages[other_page_n].data());
		}
	}
	else
	{
		// copy each page from other
		for (int page_i = 0; page_i < other_page_n; ++page_i, first += page_size)
			std::copy_n(first, page_size, pages[page_i].data());

		std::copy_n(first, elem_n, pages[other_page_n].data());
		first += elem_n;

		// destroy remaining elements, do not deallocate
		if (page_n == other_page_n)
		{
			std::destroy_n(pages[page_n].data() + elem_n, other_elem_n - elem_n);
		}
		else
		{
			std::destroy_n(pages[other_page_n].data() + other_elem_n, page_size - other_elem_n);
			for (size_t page_i = page_n + 1; page_i < page_n; ++page_i)
				std::destroy_n(pages[page_i].data(), page_size);
			std::destroy_n(pages[page_n].data(), elem_n);
		}
	}

	extent = count;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::assign(size_t count, const T& value)
{
	if (count == 0) return;

	reserve(count);

	size_t page_n = extent / page_size;
	size_t elem_n = extent % page_size;
	size_t other_page_n = count / page_size;
	size_t other_elem_n = count % page_size;

	if (extent < count)
	{
		for (int page_i = 0; page_i < page_n; ++page_i)
			std::fill_n(pages[page_i].data(), page_size, value);
		std::fill_n(pages[page_n].data(), elem_n, value);

		if (page_n == other_page_n)
		{
			std::uninitialized_fill_n(pages[page_n].data() + elem_n, other_elem_n - elem_n, value);
		}
		else
		{
			std::uninitialized_fill_n(pages[page_n].data() + elem_n, page_size - elem_n, value);
			for (size_t page_i = page_n + 1; page_i < other_page_n; ++page_i)
				std::uninitialized_fill_n(pages[page_i].data(), page_size, value);
			std::uninitialized_fill_n(pages[other_page_n].data(), other_elem_n, value);
		}
	}
	else
	{
		// copy each page from other
		for (int page_i = 0; page_i < other_page_n; ++page_i)
			std::fill_n(pages[page_i].data(), page_size, value);
		std::fill_n(pages[other_page_n].data(), elem_n, value);

		// destroy remaining elements, do not deallocate
		if (page_n == other_page_n)
		{
			std::destroy_n(pages[page_n].data() + elem_n, other_elem_n - elem_n);
		}
		else
		{
			std::destroy_n(pages[other_page_n].data() + other_elem_n, page_size - other_elem_n);
			for (size_t page_i = page_n + 1; page_i < page_n; ++page_i)
				std::destroy_n(pages[page_i].data(), page_size);
			std::destroy_n(pages[page_n].data(), elem_n);
		}
	}

	extent = count;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::assign(std::initializer_list<T> ilist)
{
	assign(ilist.begin(), ilist.end());
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::allocator_type&
ecs::packed<T, N, Alloc_T>::get_allocator() noexcept
{
	return alloc;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::page_allocator_type&
ecs::packed<T, N, Alloc_T>::get_page_allocator() noexcept
{
	return pages.get_allocator();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::begin() noexcept
{
	return { this, 0, 0 };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_iterator
ecs::packed<T, N, Alloc_T>::begin() const noexcept
{
	return cbegin();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::end() noexcept
{
	return { this, extent };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_iterator
ecs::packed<T, N, Alloc_T>::end() const noexcept
{
	return cend();
}


template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reverse_iterator
ecs::packed<T, N, Alloc_T>::rbegin() noexcept
{
	return iterator{ this, extent - 1 };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reverse_iterator
ecs::packed<T, N, Alloc_T>::rbegin() const noexcept
{
	return crbegin();
}



template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reverse_iterator
ecs::packed<T, N, Alloc_T>::rend() noexcept
{
	return iterator{ this, -1, page_size - 1 };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reverse_iterator
ecs::packed<T, N, Alloc_T>::rend() const noexcept
{
	return crend();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_iterator
ecs::packed<T, N, Alloc_T>::cbegin() const noexcept
{
	return { this, 0, 0 };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_iterator
ecs::packed<T, N, Alloc_T>::cend() const noexcept
{
	return { this, extent };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reverse_iterator
ecs::packed<T, N, Alloc_T>::crbegin() const noexcept
{
	return const_iterator{ this, extent - 1 };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reverse_iterator
ecs::packed<T, N, Alloc_T>::crend() const noexcept
{
	return const_iterator{ this, -1, page_size - 1 }; // last element of page -1
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed<T, N, Alloc_T>::empty() const noexcept
{
	return extent == 0;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr size_t ecs::packed<T, N, Alloc_T>::size() const
{
	return extent;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr size_t ecs::packed<T, N, Alloc_T>::page_count() const
{
	return pages.size();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr size_t ecs::packed<T, N, Alloc_T>::capacity() const
{
	return page_count() * page_size;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::resize(size_t n)
{
	size_t page_n = extent / page_size;
	size_t elem_n = extent % page_size;
	size_t new_page_n = n / page_size;
	size_t new_elem_n = n % page_size;

	if (extent < n)
	{
		reserve(n);
		
		if (page_n == new_page_n)
		{
			std::uninitialized_default_construct_n(pages[page_n].data() + elem_n, new_elem_n - elem_n);
		}
		else
		{
			std::uninitialized_default_construct_n(pages[page_n].data() + elem_n, page_size - elem_n);
			for (size_t page_i = page_n + 1; page_i < new_page_n; ++page_i)
				std::uninitialized_default_construct_n(pages[page_i].data(), page_size);
			std::uninitialized_default_construct_n(pages[new_page_n].data(), new_elem_n);
		}
	}
	else
	{
		if (page_n == new_page_n)
		{
			std::destroy_n(pages[page_n].data() + elem_n, elem_n - new_elem_n);
		}
		else
		{
			std::destroy_n(pages[new_page_n].data() + new_elem_n, page_size - new_elem_n);
			for (size_t page_i = page_n + 1; page_i < page_n; ++page_i)
				std::destroy_n(pages[page_i].data(), page_size);
			std::destroy_n(pages[page_n].data(), elem_n);
		}
	}

	extent = n;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::resize(size_t n, const T& value)
{
	size_t page_n = extent / page_size;
	size_t elem_n = extent % page_size;
	size_t new_page_n = n / page_size;
	size_t new_elem_n = n % page_size;

	if (extent < n)
	{
		reserve(n);
	
		if (page_n == new_page_n)
		{
			std::uninitialized_fill_n(pages[page_n].data() + elem_n, new_elem_n - elem_n, value);
		}
		else
		{
			std::uninitialized_fill_n(pages[page_n].data() + elem_n, page_size - elem_n, value);
			for (size_t page_i = page_n + 1; page_i < new_page_n; ++page_i)
				std::uninitialized_fill_n(pages[page_i].data(), page_size, value);
			std::uninitialized_fill_n(pages[new_page_n].data(), new_elem_n, value);
		}
	}
	else
	{
		if (page_n == new_page_n)
		{
			std::destroy_n(pages[page_n].data() + elem_n, new_elem_n - elem_n);
		}
		else
		{
			std::destroy_n(pages[new_page_n].data() + new_elem_n, page_size - new_elem_n);
			for (size_t page_i = page_n + 1; page_i < page_n; ++page_i)
				std::destroy_n(pages[page_i].data(), page_size);
			std::destroy_n(pages[page_n].data(), elem_n);
		}
	}

	extent = n;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::reserve(size_t n)
{
	if (n == 0) return;
	
	size_t count = ((n - 1) / page_size) + 1;
	
	if (count > page_count())
	{
		// allocate pages
		pages.reserve(count);
		while(page_count() != count)
			pages.emplace_back(get_allocator().allocate(page_size), page_size);
	}
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::shrink_to_fit()
{
	size_t page_end = (extent / page_size);
	// deallocate pages
	for (int page_i = extent / page_size; page_i != page_end; --page_i)
	{
		get_allocator().deallocate(pages[page_i].data(), page_size);
		pages.pop_back();
	}

	pages.shrink_to_fit();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reference
ecs::packed<T, N, Alloc_T>::operator[](size_t index)
{
	return pages[index / page_size][index % page_size];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reference
ecs::packed<T, N, Alloc_T>::operator[](size_t index) const
{
	return pages[index / page_size][index % page_size];
}

// element access
template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reference
ecs::packed<T, N, Alloc_T>::at(size_t pos)
{
	if (pos >= this->size()) throw std::out_of_range("paged vector index out of range");
	return pages[pos / page_size][pos % page_size];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reference
ecs::packed<T, N, Alloc_T>::at(size_t pos) const
{
	if (pos >= this->size()) throw std::out_of_range("paged vector index out of range");
	return pages[pos / page_size][pos % page_size];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reference
ecs::packed<T, N, Alloc_T>::front()
{
	return pages[0][0];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reference
ecs::packed<T, N, Alloc_T>::front() const
{
	return pages[0][0];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::reference
ecs::packed<T, N, Alloc_T>::back()
{
	return at(extent - 1);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::const_reference
ecs::packed<T, N, Alloc_T>::back() const
{
	return at(extent - 1);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::page_type*
ecs::packed<T, N, Alloc_T>::data() noexcept
{
	return pages.data();
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr const ecs::packed<T, N, Alloc_T>::page_type*
ecs::packed<T, N, Alloc_T>::data() const noexcept
{
	return pages.data();
}

template<typename T, std::size_t N, typename Alloc_T> template<typename ... Arg_Ts>
constexpr ecs::packed<T, N, Alloc_T>::reference
ecs::packed<T, N, Alloc_T>::emplace_back(Arg_Ts&&... args)
{
	reserve(++extent);
	std::construct_at<T>(&back(), std::forward<Arg_Ts>(args)...);
	return back();
}


template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::push_back(const T& value)
{
	size_t index = extent;
	reserve(++extent);
	std::construct_at(&pages[index / extent][index % page_size], value);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::push_back(T&& value)
{
	size_t index = extent;
	reserve(++extent);
	std::construct_at(&pages[index / page_size][index % page_size], value);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::pop_back()
{
	std::destroy_at(&pages[extent / page_size][extent % page_size]);
	--extent;
}

template<typename T, std::size_t N, typename Alloc_T> template<typename ... Arg_Ts>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::emplace(const_iterator pos, Arg_Ts&&... args)
{
	reserve(extent + 1);
	iterator it = begin() + (pos - cbegin());
	std::move_backward(it, end(), end() + 1);
	std::construct_at<T>(&(*it), std::forward<Arg_Ts>(args)...);
	++extent;
	return it;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::insert(const_iterator pos, const T& value)
{
	return emplace(pos, value);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::insert(const_iterator pos, T&& value)
{
	return emplace(pos, std::move(value));
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::insert(const_iterator pos, size_t n, const T& value)
{
	iterator it = begin() + (pos - cbegin());
	reserve(extent + n);
	std::move_backward(it, end(), end() + n);
	std::uninitialized_fill_n(it, n, value);
	extent += n;
	return it;
}

template<typename T, std::size_t N, typename Alloc_T> template<std::input_iterator It>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::insert(const_iterator pos, It first, It last)
{
	size_t n = last - first;
	iterator it = begin() + (pos - cbegin());
	reserve(extent + n);
	std::move_backward(pos, cend(), end() + n);
	std::uninitialized_copy(first, last, it);
	extent += n;
	return it;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::insert(const_iterator pos, std::initializer_list<T> ilist)
{
	return insert(pos, ilist.begin(), ilist.end());
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::erase(const_iterator pos)
{
	iterator it1 = begin() + (pos - cbegin());
	iterator it2 = it1 + 1;

	if (it2 != end()) std::move(it2, end(), it1);
	else std::destroy_at(&*it1);

	extent -= 1;
	return it1;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed<T, N, Alloc_T>::iterator
ecs::packed<T, N, Alloc_T>::erase(const_iterator first, const_iterator last)
{
	size_t n = (last - first);
	iterator it1 = begin() + (first - cbegin());
	iterator it2 = it1 + (last - cbegin());

	if (it2 != end()) std::move(it2, end(), it1);
	else std::destroy(it1, it2);

	extent -= n;
	return it1;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::swap(packed& other)
{
	std::swap(extent, other.extent);
	std::swap(pages, other.pages);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr void ecs::packed<T, N, Alloc_T>::clear() noexcept
{
	if (extent == 0) return;

	size_t page_n = extent / page_size;
	size_t elem_n = extent % page_size;
	for (int page_i = 0; page_i < page_n; ++page_i)
		std::destroy_n(pages[page_i].data(), page_size);
	std::destroy_n(pages[page_n].data(), elem_n);
	
	extent = 0;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::packed_iterator()
 : range(nullptr), page_index(-1), elem_index(-1) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::packed_iterator(base_range* range, size_t index)
 : range(range), page_index(index / page_size), elem_index(index % page_size) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::packed_iterator(base_range* range, std::size_t page_index, std::size_t elem_index)
 : range(range), page_index(page_index), elem_index(elem_index) { }

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::operator
packed_iterator<const T, N, Alloc_T>() const
{
	return packed_iterator<const T, N, Alloc_T>{ range, page_index, elem_index };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::reference
ecs::packed_iterator<T, N, Alloc_T>::operator*() const
{
	return (*range).pages[page_index][elem_index];
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::reference
ecs::packed_iterator<T, N, Alloc_T>::operator[](difference_type n) const
{
	return *(*this + n);
}


template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator==(const packed_iterator& other) const
{
	return page_index == other.page_index && elem_index == other.elem_index;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator!=(const packed_iterator& other) const
{
	return page_index != other.page_index || elem_index != other.elem_index;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator<(const packed_iterator& other) const
{
	return page_index < other.page_index || (page_index == other.page_index && elem_index < other.elem_index);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator<=(const packed_iterator& other) const
{
	return page_index < other.page_index || (page_index == other.page_index && elem_index <= other.elem_index);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator>(const packed_iterator& other) const
{
	return page_index > other.page_index || (page_index == other.page_index && elem_index > other.elem_index);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr bool ecs::packed_iterator<T, N, Alloc_T>::operator>=(const packed_iterator& other) const
{
	return page_index > other.page_index || (page_index == other.page_index && elem_index >= other.elem_index);
}



template<typename T, std::size_t N, typename Alloc_T>
ecs::packed_iterator<T, N, Alloc_T> operator+(
	typename ecs::packed_iterator<T, N, Alloc_T>::difference_type n,
	const ecs::packed_iterator<T, N, Alloc_T>& it)
{
	return it + n;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>&
ecs::packed_iterator<T, N, Alloc_T>::operator++()
{
	if (elem_index == page_size - 1)
	{
		elem_index = 0;
		++page_index;
	}
	else
	{
		++elem_index;
	}
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>
ecs::packed_iterator<T, N, Alloc_T>::operator++(int)
{
	packed_iterator temp = *this; ++(*this); return temp;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>&
ecs::packed_iterator<T, N, Alloc_T>::operator--()
{
	if (elem_index == 0)
	{
		elem_index = page_size - 1;
		--page_index;
	}
	else
	{
		--elem_index;
	}
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>
ecs::packed_iterator<T, N, Alloc_T>::operator--(int)
{
	packed_iterator temp = *this; --(*this); return temp;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>
ecs::packed_iterator<T, N, Alloc_T>::operator+(difference_type n) const
{
	return { range, (page_index * page_size) + elem_index + n };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>
ecs::packed_iterator<T, N, Alloc_T>::operator-(difference_type n) const
{
	return { range, (page_index * page_size) + elem_index - n };
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>&
ecs::packed_iterator<T, N, Alloc_T>::operator+=(difference_type n)
{
	size_t index = (page_index * page_size) + elem_index + n;
	page_index = index / page_size;
	elem_index = index % page_size;
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>&
ecs::packed_iterator<T, N, Alloc_T>::operator-=(difference_type n)
{
	size_t index = (page_index * page_size) + elem_index - n;
	page_index = index / page_size;
	elem_index = index % page_size;
	return *this;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr ecs::packed_iterator<T, N, Alloc_T>::difference_type
ecs::packed_iterator<T, N, Alloc_T>::operator-(const packed_iterator& other) const
{
	return ((page_index - other.page_index) * page_size) + (elem_index - other.elem_index);
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr size_t ecs::packed_iterator<T, N, Alloc_T>::get_index() const
{
	return (page_index * page_size) + elem_index;
}

template<typename T, std::size_t N, typename Alloc_T>
constexpr std::pair<std::size_t, std::size_t> ecs::packed_iterator<T, N, Alloc_T>::get_indices() const
{
	return { page_index, elem_index };
}

static_assert(std::ranges::random_access_range<ecs::packed<int>>);
static_assert(std::random_access_iterator<ecs::packed<int>::iterator>);