#pragma once
#include "core/traits.h"
#include "util/view_element.h"
#include <stdexcept>

// pool iterator
namespace ecs {
	template<ecs::traits::component_class T, typename reg_T>
	class pool_iterator {
	public:
		using entity_type = typename component_traits<std::remove_const_t<T>>::entity_type;
		using handle_type = typename entity_traits<entity_type>::handle_type;

		using manager_type = typename component_traits<std::remove_const_t<T>>::manager_type;
		using indexer_type = typename component_traits<std::remove_const_t<T>>::indexer_type;
		using storage_type = typename component_traits<std::remove_const_t<T>>::storage_type;

		using iterator_category = std::random_access_iterator_tag;
		using value_type = ecs::view_element<handle_type, T&>;
		using difference_type = std::ptrdiff_t;

		pool_iterator() = default;
		pool_iterator(std::size_t pos, reg_T* reg) : pos(pos), reg(reg) { }
		
		pool_iterator(const pool_iterator& other) = default;
		pool_iterator& operator=(const pool_iterator& other) = default;
		pool_iterator(pool_iterator&& other) = default;
		pool_iterator& operator=(pool_iterator&& other) = default;

		operator pool_iterator<const T, const reg_T>() const { return { reg, pos }; }

		constexpr value_type operator*() const {
			return {
				reg->template get_resource<manager_type>()[pos],
				reg->template get_resource<storage_type>()[pos]
			};
		}
		constexpr value_type operator[](difference_type n) const {
			n += pos;
			return {
				reg->template get_resource<manager_type>()[n],
				reg->template get_resource<storage_type>()[n]
			}; 
		}

		inline constexpr bool operator==(const pool_iterator& other) const { return pos == other.pos; }
		inline constexpr bool operator!=(const pool_iterator& other) const { return pos != other.pos; }
		inline constexpr bool operator<(const pool_iterator& other) const  { return pos < other.pos; }
		inline constexpr bool operator>(const pool_iterator& other) const  { return pos > other.pos; }
		inline constexpr bool operator<=(const pool_iterator& other) const { return pos <= other.pos; }
		inline constexpr bool operator>=(const pool_iterator& other) const { return pos >= other.pos; }

		inline constexpr pool_iterator& operator++() { ++pos; return *this; }
		inline constexpr pool_iterator& operator--() { --pos; return *this; }
		inline constexpr pool_iterator operator++(int) { auto tmp = *this; ++pos; return tmp; }
		inline constexpr pool_iterator operator--(int) { auto tmp = *this; --pos; return tmp; }
		inline constexpr pool_iterator& operator+=(difference_type n) { pos += n; return *this; }
		inline constexpr pool_iterator& operator-=(difference_type n) { pos -= n; return *this; }
		
		inline constexpr pool_iterator operator+(difference_type n) const { auto tmp = *this; return tmp += n; }
		inline constexpr pool_iterator operator-(difference_type n) const { auto tmp = *this; return tmp -= n; }

		inline constexpr difference_type operator-(const pool_iterator& other) const { return pos - other.pos; }

		inline constexpr std::size_t index() const { return pos; }
	private:
		std::size_t pos;
		reg_T* reg;
	};

	template<typename T, typename reg_T>
	pool_iterator<T, reg_T> operator+(typename pool_iterator<T, reg_T>::difference_type n, const pool_iterator<T, reg_T>& it) { 
		return it.pos + n;
	}
}

// pool
namespace ecs
{
	template<ecs::traits::component_class T, typename reg_T>
	class pool {
	public:
		using registry_type = reg_T;
		
		using entity_type = typename component_traits<std::remove_const_t<T>>::entity_type;
		using handle_type = typename entity_traits<entity_type>::handle_type;
		using value_view = typename handle_type::value_view;
		using version_view = typename handle_type::version_view;
		using integral_type = typename handle_type::integral_type;
		
		using manager_type = typename component_traits<std::remove_const_t<T>>::manager_type;
		using indexer_type = typename component_traits<std::remove_const_t<T>>::indexer_type;
		using storage_type = typename component_traits<std::remove_const_t<T>>::storage_type; 
		
		using value_type = ecs::view_element<handle_type, T&>;
		using reference = ecs::view_element<handle_type, T&>;
		using const_reference = ecs::view_element<handle_type, const T&>;
		using iterator = pool_iterator<T, reg_T>;
		using const_iterator = pool_iterator<const T, const reg_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	private:
		static constexpr bool enable_term_event = traits::is_accessible_v<event::term<T>, registry_type>;
		static constexpr bool enable_init_event = traits::is_accessible_v<event::init<T>, registry_type>;
		static constexpr bool enable_indexer = !std::is_same_v<indexer_type, void>;
		static constexpr bool enable_storage = !std::is_same_v<storage_type, void>;
	public:
		inline constexpr pool(reg_T* reg) noexcept : reg(reg) { }

		// iterators
		inline constexpr iterator begin() noexcept { return { 0, reg }; }
		inline constexpr const_iterator begin() const noexcept { return { 0, reg }; }
		inline constexpr const_iterator cbegin() const noexcept { return { 0, reg }; }

		inline constexpr iterator end() noexcept { return { reg->template get_resource<manager_type>().size(), reg }; }
		inline constexpr const_iterator end() const noexcept { return { reg->template get_resource<manager_type>().size() }; }
		inline constexpr const_iterator cend() const noexcept { return { reg->template get_resource<manager_type>().size(), reg }; }
		
		inline constexpr reverse_iterator rbegin() noexcept { return iterator{ reg->template get_resource<manager_type>().size()-1 , reg}; }
		inline constexpr const_reverse_iterator rbegin() const noexcept { return const_iterator{ reg->template get_resource<manager_type>().size()-1, reg }; }
		inline constexpr const_reverse_iterator crbegin() const noexcept { return const_iterator{ reg->template get_resource<manager_type>().size()-1, reg }; }
		
		inline constexpr reverse_iterator rend() noexcept { return iterator{ -1, reg }; }
		inline constexpr const_reverse_iterator rend() const noexcept { return const_iterator{ -1, reg }; }
		inline constexpr const_reverse_iterator crend() const noexcept { return const_iterator{ -1, reg }; }

		// capacity
		[[nodiscard]] inline constexpr bool empty() const noexcept { return reg->template get_resource<manager_type>().size() == 0; }
		inline constexpr std::size_t max_size() const noexcept { return reg->template get_resource<manager_type>().max_size(); }
		inline constexpr std::size_t size() const { return reg->template get_resource<manager_type>().size(); }
		inline constexpr std::size_t capacity() const { return reg->template get_resource<manager_type>().capacity(); }
		
		constexpr void reserve(std::size_t n) {
			reg->template get_resource<manager_type>().reserve(n);
			if constexpr (enable_storage) {
				reg->template get_resource<storage_type>().reserve(n);
			}
		}
		constexpr void shrink_to_fit() {
			std::size_t n = size();
			reg->template get_resource<manager_type>().shrink_to_fit(n);
			if constexpr (enable_storage) {
				reg->template get_resource<storage_type>().shrink_to_fit(n);
			}
		}
		
		// element/page/data access
		inline constexpr reference operator[](std::size_t pos) {
			if constexpr (enable_storage) {
				return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] };
			} else {
				return { reg->template get_resource<manager_type>()[pos] };
			}
		}	
		inline constexpr reference operator[](handle_type ent) {
			if constexpr (enable_indexer) {
				auto key = value_type{ ent };
				auto pos = reg->template get_resource<indexer_type>().at(key);
				return operator[](value_type{ pos });
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				return operator[](std::ranges::find(manager, ent) - manager.begin());
			}
		}
		
		constexpr reference at(std::size_t pos) {
			if (pos >= size()) throw std::out_of_range("");
			return operator[](pos);
		}

		constexpr T& get_component(handle_type ent) requires (enable_storage) {
			return reg->template get_resource<storage_type>().at(index_of(ent));
		}

		constexpr reference front() {
			if (empty()) throw std::out_of_range("");
			return operator[](0);
		}
		
		constexpr reference back() {
			if (empty()) throw std::out_of_range("");
			return operator[](size() - 1);
		}

		[[nodiscard]] constexpr integral_type index_of(handle_type ent) {
			if constexpr (enable_indexer) {
				auto& indexer = reg->template get_resource<indexer_type>();
				if (auto it = indexer.find(value_view{ ent }); it != indexer.end()) {
					if (version_view{ it->second } == version_view{ ent }) {
						return value_view{ it->second };
					}
				}
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				if (auto it = std::ranges::find(manager, ent); it != manager.end()) {
					return it - manager.begin();
				}
			}
			return -1;
		}

		[[nodiscard]] bool contains(handle_type ent) {
			if constexpr (enable_indexer) {
				auto& indexer = reg->template get_resource<indexer_type>();
				auto it = indexer.find(value_view{ ent });
				return (it != indexer.end()) && (version_view { it->second } == version_view{ ent });
			} else {
				auto& manager = reg->template get_resource<manager_type>();
				return std::ranges::find(manager, ent) != manager.end();
			}
		}

		// modifiers
		template<typename ... arg_Ts> requires (std::is_constructible_v<T, arg_Ts...>)
		std::conditional_t<enable_storage, T&, void> 
		emplace(handle_type ent, arg_Ts&&... args) {
			integral_type back = size();

			if constexpr (enable_indexer) {
				auto& indexer = reg->template get_resource<indexer_type>();
				indexer[value_view{ ent }] = handle_type{ back, ent };
			}

			reserve(back + 1);
			
			
			auto& manager = reg->template get_resource<manager_type>();
			manager.emplace_back(ent);
			
			if constexpr (enable_storage) {
				auto& storage = reg->template get_resource<storage_type>();
				auto& component = storage.emplace_back(std::forward<arg_Ts>(args)...);
				
				if constexpr (enable_init_event) {
					reg->template on<event::init<T>>().invoke(ent, component);
				}

				return component;
			} else {
				if constexpr (enable_init_event) {
					reg->template on<event::init<T>>().invoke(ent);
				}
			}
		}

		constexpr bool erase(handle_type ent) {
			integral_type back = static_cast<integral_type>(size() - 1);
			integral_type pos = index_of(ent);
			
			if (pos == -1) return false;
			
			if constexpr (enable_indexer) {
				auto& indexer = reg->template get_resource<indexer_type>();
				indexer.erase(value_view{ ent });
			}
			
			if (pos != back) {
				if constexpr (enable_indexer) {
					auto& indexer = reg->template get_resource<indexer_type>();
					handle_type back_ent = reg->template get_resource<manager_type>()[back];
					indexer.at(value_view{ back_ent }) = { pos, version_view{ back_ent } }; // update value, keep version
				}
				std::swap(
					reg->template get_resource<manager_type>()[pos], 
					reg->template get_resource<manager_type>()[back]
				);
				if constexpr (enable_storage) {
					std::swap(
						reg->template get_resource<storage_type>()[pos], 
						reg->template get_resource<storage_type>()[back]
					);
				}
			}

			reg->template get_resource<manager_type>().pop_back();
			if constexpr (enable_storage) {
				reg->template get_resource<storage_type>().pop_back();
			}
			
			if constexpr (enable_term_event) {
				if constexpr (enable_storage) {
					reg->template on<event::term<T>>().invoke(ent, reg->template get_resource<storage_type>()[pos]);
				} else {
					reg->template on<event::term<T>>().invoke(ent);
				}
			}

			return true;
		}

		constexpr void clear() {			
			if constexpr (enable_term_event) {
				for (std::size_t pos = 0; pos < size(); ++pos) {
					if constexpr (enable_storage) {
						reg->template on<event::term<T>>().invoke(
							reg->template get_resource<manager_type>()[pos], 
							reg->template get_resource<storage_type>()[pos]
						);
					} else {
						reg->template on<event::term<T>>().invoke(
							reg->template get_resource<manager_type>()[pos]
						);
					}
				}
			}

			
			reg->template get_resource<manager_type>().clear();

			if constexpr (enable_indexer) {
				reg->template get_resource<indexer_type>().clear();
			}
			if constexpr (enable_storage) {
				reg->template get_resource<storage_type>().clear();
			}
		}

	private:
		reg_T* reg;
	};
}
