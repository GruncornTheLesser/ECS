#pragma once
#include <vector>
#include <array>

namespace ecs {
    template<typename T, std::size_t page_size=4096, typename Alloc_T=std::allocator<T>>
    struct sparse { 
        using page = std::array<T, page_size>*;
        
        // find(key), end()
        // insert(key, value)
        // erase(key)
        // at(key)
        // operator[](key)
        // contains(key)


    private:
        std::vector<page> pages;
    };

    
}