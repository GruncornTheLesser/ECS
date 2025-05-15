#pragma once
#include <stdint.h>
#include <iterator>

namespace ecs {
    template<typename value_T>
    struct chain_node {
        operator value_T&() { return value; }
        operator const value_T&() const { return value; }

        value_T value;
        uint32_t current = 0;
    };

    template<typename Range_T>
    class chain_iterator;

    template<typename Range_T>
    class chain;
    
    template<typename Range_T>
    class chain_view;

    class chain_sentinel { };

    template<typename It_T>
    class chain_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;	

        using value_type = std::remove_reference_t<decltype(*std::declval<It_T>())>;
		using pointer = decltype(&(*std::declval<It_T>()));
		using reference = decltype(*std::declval<It_T>());
		using difference_type = decltype(std::declval<It_T>() - std::declval<It_T>());

		constexpr chain_iterator() { }
		constexpr chain_iterator(It_T it) : it(it) { }
		constexpr operator It_T() const { return it; }
			
		constexpr reference operator*() const { return *it; }
        constexpr pointer operator->() const { return it.operator->(); }

		constexpr chain_iterator& operator++() { ; return *this; }
		constexpr chain_iterator operator++(int) { auto tmp = *this; ++(*this); return *this; }

		constexpr bool operator==(It_T rhs) const { return it == rhs.it; }
		constexpr bool operator!=(It_T rhs) const { return it != rhs.it; }

        constexpr bool operator==(chain_sentinel) const { return it->version == current; }
		constexpr bool operator!=(chain_sentinel) const { return it->version != current; }
    private:
        It_T it;
        uint32_t current;
    };

    template<typename It_T> 
    bool operator==(chain_sentinel, const chain_iterator<It_T>& it) { 
        return it->version == it.current; 
    }

    template<typename It_T> 
    bool operator!=(chain_sentinel, const chain_iterator<It_T>& it) { 
        return it->version != it.current;
    }

    template<typename Range_T>
    class chain : public Range_T {
        friend class chain_view<Range_T>;
    private:
        uint32_t head = 0xffffffff;
        uint32_t current = 0;
    }; 

    template<typename Range_T>
    class chain_view {
    public:
        using chain_type = chain<Range_T>;

        using value_type = typename Range_T::value_type;
        using pointer_type = value_type*;
        using reference_type = value_type&;
        using iterator = chain_iterator<typename chain_type::iterator>;
        using const_iterator = chain_iterator<typename chain_type::const_iterator>;
        using sentinel_type = chain_sentinel;

        chain_view(chain_type& chain) : chain(chain) { }

		constexpr iterator begin() { return chain.begin() + chain.head; }
		constexpr const_iterator begin() const { return chain.begin() + chain.head; }
        constexpr chain_sentinel end() const { return { }; }

        void push(Range_T::iterator it) { }
        void clear();
		
    private:
        chain_type& chain;
    };
}


#include <array>
static_assert(std::ranges::range<ecs::chain_view<std::array<ecs::chain_node<int>, 4096>>>);
static_assert(std::ranges::forward_range<ecs::chain_view<std::array<ecs::chain_node<int>, 4096>>>);
static_assert(std::forward_iterator<typename ecs::chain_view<std::array<ecs::chain_node<int>, 4096>>::iterator>);