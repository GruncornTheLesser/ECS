#pragma once
#include <stdexcept>
#include "traits.h"
#include "element.h"

namespace ecs {
	template<typename T> struct manager
	 : std::vector<typename table_traits<typename component_traits<T>::table_type>::handle_type> 
	{ using ecs_tag = ecs::tag::resource; };
	template<typename T> struct indexer
	 : std::vector<typename table_traits<typename component_traits<T>::table_type>::handle_type> 
	{ using ecs_tag = ecs::tag::resource; };
	template<typename T> struct storage
	 : std::vector<T> { using ecs_tag = ecs::tag::resource; };
}

// pool iterator
namespace ecs {
	template<typename T, typename reg_T>
	class pool_iterator {
	public:
		using table_type = typename component_traits<std::remove_const_t<T>>::table_type;
		using handle_type = typename table_traits<table_type>::handle_type;
		using handle_value_type = typename handle_traits<handle_type>::value_type;
		using handle_version_type = typename handle_traits<handle_type>::version_type;
		using handle_integral_type = typename handle_traits<handle_type>::integral_type;

		using manager_type = typename component_traits<std::remove_const_t<T>>::manager_type;
		using indexer_type = typename component_traits<std::remove_const_t<T>>::indexer_type;
		using storage_type = typename component_traits<std::remove_const_t<T>>::storage_type;

		using iterator_category = std::random_access_iterator_tag;
		using value_type = ecs::element<handle_type, T&>;
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
	template<typename T, typename reg_T>
	class pool {
	public:
		using registry_type = reg_T;
		
		using table_type = typename component_traits<std::remove_const_t<T>>::table_type;
		using handle_type = typename table_traits<table_type>::handle_type;
		using handle_value_type = typename handle_traits<handle_type>::value_type;
		using handle_version_type = typename handle_traits<handle_type>::version_type;
		using handle_integral_type = typename handle_traits<handle_type>::integral_type;
		
		using manager_type = typename component_traits<std::remove_const_t<T>>::manager_type;
		using indexer_type = typename component_traits<std::remove_const_t<T>>::indexer_type;
		using storage_type = typename component_traits<std::remove_const_t<T>>::storage_type; 
		
		using value_type = ecs::element<handle_type, T&>;
		using reference = ecs::element<handle_type, T&>;
		using const_reference = ecs::element<handle_type, const T&>;
		using iterator = pool_iterator<T, reg_T>;
		using const_iterator = pool_iterator<const T, const reg_T>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	private:
		static constexpr bool enable_terminate_event = traits::is_accessible_v<event::terminate<T>, registry_type>;
		static constexpr bool enable_init_event = traits::is_accessible_v<event::init<T>, registry_type>;
		
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
			reg->template get_resource<storage_type>().reserve(n);
		}
		constexpr void shrink_to_fit() {
			std::size_t n = size();
			reg->template get_resource<manager_type>().shrink_to_fit(n);
			reg->template get_resource<storage_type>().shrink_to_fit(n);
		}
		
		// element/page/data access
		inline constexpr reference operator[](std::size_t pos) {
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] };
		}
		inline constexpr const_reference operator[](std::size_t pos) const {
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] }; 
		}
		
		inline constexpr reference operator[](handle_type ent) {
			std::size_t key = handle_value_type { ent };
			std::size_t pos = handle_value_type{ reg->template get_resource<indexer_type>()[key] };
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] }; 
		}
		inline constexpr const_reference operator[](handle_type ent) const {
			std::size_t key = handle_value_type{ ent };
			std::size_t pos = handle_value_type{ reg->template get_resource<indexer_type>()[key] };
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] }; 
		}
		
		constexpr reference at(std::size_t pos) {
			if (pos >= size()) throw std::out_of_range("");
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] }; 
		}
		constexpr const_reference at(std::size_t pos) const {
			if (pos >= size()) throw std::out_of_range("");
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] }; 
		}

		constexpr T& get_component(handle_type ent) {
			auto pos = index_of(ent);
			return reg->template get_resource<storage_type>()[pos];
		}
		constexpr const T& get_component(handle_type ent) const {
			auto pos = index_of(ent);
			return reg->template get_resource<storage_type>()[pos]; 
		}

		constexpr reference front() {
			if (empty()) throw std::out_of_range("");
			return { reg->template get_resource<manager_type>()[0], reg->template get_resource<storage_type>()[0] }; 
		}
		constexpr const_reference front() const {
			if (empty()) throw std::out_of_range("");
			return { reg->template get_resource<manager_type>()[0], reg->template get_resource<storage_type>()[0] };
		}
		
		constexpr reference back() {
			if (empty()) throw std::out_of_range("");
			std::size_t pos = size() - 1;
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] };
		}
		constexpr const_reference back() const {
			if (empty()) throw std::out_of_range("");
			std::size_t pos = size() - 1;
			return { reg->template get_resource<manager_type>()[pos], reg->template get_resource<storage_type>()[pos] };
		}
		
		constexpr std::size_t index_of(handle_type ent) const {
			std::size_t key = handle_value_type{ ent };
			if (key >= reg->template get_resource<indexer_type>().size()) throw std::out_of_range("");

			auto ind = reg->template get_resource<indexer_type>()[key];
			if (handle_version_type{ ent } != handle_version_type{ ind }) throw std::out_of_range("");

			std::size_t pos = handle_value_type{ ind };
			if (pos >= size()) throw std::out_of_range("");

			return pos;
		}

		[[nodiscard]] bool contains(handle_type ent) const {
			std::size_t key = handle_value_type{ ent };
			if (key >= reg->template get_resource<indexer_type>().size()) return false;
			
			auto ind = reg->template get_resource<indexer_type>()[key];
			if (handle_version_type{ ent } != handle_version_type{ ind }) return false;
			
			std::size_t pos = handle_value_type{ ind };
			if (pos >= size()) return false;

			return reg->template get_resource<manager_type>()[pos] == ent;
		}

		// modifiers
		template<typename ... arg_Ts> requires (std::is_constructible_v<T, arg_Ts...>)
		constexpr T& emplace(handle_type ent, arg_Ts&&... args) {
			std::size_t key = handle_value_type{ ent };
			
			auto& indexer = reg->template get_resource<indexer_type>();
			if (key >= indexer.size()) indexer.resize(key + 1, tombstone{}); // creates empty reference
			indexer[key] = handle_type{ static_cast<handle_integral_type>(size()), ent };
			
			auto& manager = reg->template get_resource<manager_type>();
			auto& storage = reg->template get_resource<storage_type>();

			manager.emplace_back(ent);
			T& component = storage.emplace_back(std::forward<arg_Ts>(args)...);
			
			if constexpr (enable_init_event) {
				reg->template on<event::init<T>>().invoke({ ent, component });
			}
			return component;
		}

		template<typename ... arg_Ts> requires (std::is_constructible_v<T, arg_Ts...>)
		T& get_or_emplace(handle_type ent, arg_Ts&& ... args)
		{
			std::size_t key = handle_value_type{ ent };

			auto& indexer = reg->template get_resource<indexer_type>();
			if (key >= indexer.size()) {
				indexer.resize(key + 1, tombstone{}); // creates empty reference
				indexer[key] = handle_type{ static_cast<handle_integral_type>(size()), ent };

				auto& manager = reg->template get_resource<manager_type>();
				auto& storage = reg->template get_resource<storage_type>();

				manager.emplace_back(ent);
				T& component = storage.emplace_back(std::forward<arg_Ts>(args)...);

				if constexpr (enable_init_event) {
					reg->template on<event::init<T>>().invoke({ ent, component });
				}

				return component;
			}
			
			auto ind = reg->template get_resource<indexer_type>()[key];

			if (handle_version_type{ key } == handle_version_type{ ind }) {
				indexer[key] = handle_type{ static_cast<handle_integral_type>(size()), ent };
				
				auto& manager = reg->template get_resource<manager_type>();
				auto& storage = reg->template get_resource<storage_type>();

				manager.emplace_back(ent);
				T& component = storage.emplace_back(std::forward<arg_Ts>(args)...);
				
				if constexpr (enable_init_event) {
					reg->template on<event::init<T>>().invoke({ ent, component });
				}
				return component;
			}
			
			std::size_t pos = handle_value_type{ ind };
			return { 
				reg->template get_resource<manager_type>()[pos], 
				reg->template get_resource<storage_type>()[pos]
			}; 
		}

		constexpr void erase(handle_type ent) {
			std::size_t key = handle_value_type{ ent };
			if (key >= reg->template get_resource<indexer_type>().size()) throw std::out_of_range("");

			auto& ind = reg->template get_resource<indexer_type>()[key];
			if (handle_version_type{ ent } != handle_version_type{ ind }) throw std::out_of_range("");

			std::size_t back = size() - 1;
			std::size_t pos = handle_value_type{ ind };

			if (pos > back) throw std::out_of_range("");

			ind = tombstone{ };

			if constexpr(enable_terminate_event) {
				reg->template on<event::terminate<T>>().invoke({ ent, reg->template get_resource<storage_type>()[pos] });
			}

			if (pos != back) {
				std::swap(reg->template get_resource<storage_type>()[pos], reg->template get_resource<storage_type>()[back]);
				std::swap(reg->template get_resource<manager_type>()[pos], reg->template get_resource<manager_type>()[back]);
			}

			reg->template get_resource<storage_type>().pop_back();
			reg->template get_resource<manager_type>().pop_back();
		}
		
		bool try_erase(handle_type ent) {
			std::size_t key = handle_value_type{ ent };
			if (key >= reg->template get_resource<indexer_type>().size()) return false;

			auto& ind = reg->template get_resource<indexer_type>()[key];
			if (handle_version_type{ ent } != handle_version_type{ ind }) return false;

			std::size_t back = size() - 1;
			std::size_t pos = handle_value_type{ ind };
			
			if (pos > back) return false;

			ind = tombstone{ };

			if constexpr(enable_terminate_event) {
				reg->template on<event::terminate<T>>().invoke({ ent, reg->template get_resource<storage_type>()[pos] });
			}

			if (pos != back) {
				std::swap(reg->template get_resource<storage_type>()[pos], reg->template get_resource<storage_type>()[back]);
				std::swap(reg->template get_resource<manager_type>()[pos], reg->template get_resource<manager_type>()[back]);
			}

			reg->template get_resource<storage_type>().pop_back();
			reg->template get_resource<manager_type>().pop_back();

			return true;
		}

		constexpr void clear() noexcept {
			if constexpr (enable_terminate_event) {
				for (std::size_t pos = 0; pos < size(); ++pos) {
					reg->template on<event::terminate<T>>().invoke({ 
						reg->template get_resource<manager_type>()[pos], 
						reg->template get_resource<storage_type>()[pos] 
					});
				}
			}

			for (std::size_t pos = 0; pos < size(); ++pos) {
				std::size_t key = handle_value_type{ reg->template get_resource<manager_type>()[pos] };
				reg->template get_resource<indexer_type>()[key] = handle_value_type{ tombstone{} };
			}

			reg->template get_resource<storage_type>().clear();
			reg->template get_resource<manager_type>().clear();
		}

	private:
		reg_T* reg;
	};
}
