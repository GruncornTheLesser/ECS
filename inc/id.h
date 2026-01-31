#pragma once
#include <cstdint>
#include <string_view>

namespace ecs {
	struct id {
		template<typename T>
		static consteval std::string_view get_name() {
		#ifdef __clang__
			std::size_t prefix = sizeof("static std::string_view ecs::id::get_name() [T = ") - 1;
			std::size_t suffix = sizeof("]");
			const char* data = __PRETTY_FUNCTION__;
			// "static std::string_view ecs::id::get_name() [T = int]"
		#elif defined(__GNUC__)
			std::size_t prefix = sizeof("static constexpr std::string_view ecs::id::get_name() [with T = ") - 1;
			std::size_t suffix = sizeof("; std::string_view = std::basic_string_view<char>]");
			const char* data = __PRETTY_FUNCTION__;
			// "static constexpr std::string_view ecs::id::get_name() [with T = int; std::string_view = std::basic_string_view<char>]"
		#elif defined(_MSC_VER)
			std::size_t prefix = sizeof("class std::basic_string_view<char,struct std::char_traits<char> > __cdecl ecs::id::get_name<");
			std::size_t suffix = sizeof(">(void)");
			const char* data = __FUNCSIG__;
			return __FUNCSIG__;
			// "class std::basic_string_view<char,struct std::char_traits<char> > __cdecl ecs::id::get_name<int>(void)"
		#else
		#error "compiler not recognized."
		#endif
			std::size_t length = sizeof(__PRETTY_FUNCTION__) - prefix - suffix;
			return { data + prefix, length };
		}

		static consteval std::size_t get_hash(const std::string_view str) { 
			// Fx Hash
			const std::size_t rotate = 5;
			const std::size_t seed = sizeof(std::size_t) == 64 ? 0x517cc1b727220a95 : 0x9e3779b9;
		
			std::size_t count = str.size();
			std::size_t index = 0;

			std::size_t hash = 0;
			if constexpr (sizeof(std::size_t) == 64) {
				while (count >= 8) {
					alignas(uint64_t) char buffer[8];
					str.copy(buffer, 8, index);
					hash = (std::rotl(hash, rotate) ^ std::bit_cast<uint64_t>(buffer)) * seed;
					count -= 8;
					index += 8;
				}

				if (count >= 4) {
					alignas(uint32_t) char buffer[4];
					str.copy(buffer, 4, index);
					hash = (std::rotl(hash, rotate) ^ std::bit_cast<uint32_t>(buffer)) * seed;
					count -= 4;
					index += 4;
				}
			} else {
				while (count >= 4) {
					alignas(uint32_t) char buffer[4];
					str.copy(buffer, 4, index);
					hash = (std::rotl(hash, rotate) ^ std::bit_cast<uint32_t>(buffer)) * seed;
					count -= 4;
					index += 4;
				}
			}

			if (count >= 2) {
				alignas(uint16_t) char buffer[2];
				str.copy(buffer, 2, index);
				hash = (std::rotl(hash, rotate) ^ std::bit_cast<uint16_t>(buffer)) * seed;
				count -= 2;
				index += 2;
			}

			if (count == 1) {
				hash = (std::rotl(hash, rotate) ^ std::bit_cast<uint8_t>(str.back())) * seed;
				count -= 1;
				index += 1;
			}
			
			return hash;
		}
		
		static constexpr std::size_t nullhash = static_cast<std::size_t>(-1);
	public:
		consteval id() : hash(nullhash) { }
		consteval id(std::type_identity<void>) : hash(nullhash) { }
		
		template<typename T>
		consteval id(std::type_identity<T>) : id(get_name<std::remove_cvref_t<T>>()) { }

		template<std::size_t N>
		consteval id(const char (&data)[N]) : id(std::string_view{ data }) { }

		consteval id(std::string_view str) : hash(get_hash(str)) { }

		constexpr friend bool operator==(const id& lhs, const id& rhs) { return lhs.hash == rhs.hash; }
		constexpr friend auto operator<=>(const id& lhs, const id& rhs) { return lhs.hash <=> rhs.hash; }
		
		std::size_t hash;
	};
}

namespace std {
	template<> 
	struct hash<ecs::id> {
		constexpr hash() = default;
		constexpr std::size_t operator()(const ecs::id& id) const {
			return id.hash;
		}
	};	
}