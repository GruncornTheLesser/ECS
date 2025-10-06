#pragma once
#include <cstdint>
#include <string_view>


namespace ecs {
	struct id {
		template<typename> friend struct std::hash;

		template<typename T>
		static consteval std::string_view get_name() {
		#ifdef __clang__
			std::size_t prefix = sizeof("static std::string_view ecs::id::get_name() [T = ") - 1;
			std::size_t suffix = sizeof("]");
			std::size_t length = sizeof(__PRETTY_FUNCTION__);
			const char* data = __PRETTY_FUNCTION__;
			// "static std::string_view ecs::id::get_name() [T = int]"
		#elif defined(__GNUC__)
			std::size_t prefix = sizeof("static constexpr std::string_view ecs::id::get_name() [with T = ") - 1;
			std::size_t suffix = sizeof("; std::string_view = std::basic_string_view<char>]");
			std::size_t length = sizeof(__PRETTY_FUNCTION__);
			const char* data = __PRETTY_FUNCTION__;
			// "static constexpr std::string_view ecs::id::get_name() [with T = int; std::string_view = std::basic_string_view<char>]"
		#elif defined(_MSC_VER)
			return __FUNCSIG__;
		#else
		#error "compiler not recognized."
		#endif
			return { data + prefix, length - suffix - prefix };
		}

		static consteval std::size_t get_hash(const std::string_view str) { 
			static constexpr std::size_t ROTATE = 5;
			static constexpr std::size_t SEED = sizeof(std::size_t) == 64 ? 0x517CC1b727220A95 : 0x9e3779b9;
		
			std::size_t count = str.size();
			std::size_t index = 0;

			std::size_t hash = 0;
			if constexpr (sizeof(std::size_t) == 64) {
				while (count >= 8) {
					alignas(uint64_t) char buffer[8];
					str.copy(buffer, 8, index);
					hash = (std::rotl(hash, ROTATE) ^ std::bit_cast<uint64_t>(buffer)) * SEED;
					count -= 8;
					index += 8;
				}

				if (count >= 4) {
					alignas(uint32_t) char buffer[4];
					str.copy(buffer, 4, index);
					hash = (std::rotl(hash, ROTATE) ^ std::bit_cast<uint32_t>(buffer)) * SEED;
					count -= 4;
					index += 4;
				}
			} else {
				while (count >= 4) {
					alignas(uint32_t) char buffer[4];
					str.copy(buffer, 4, index);
					hash = (std::rotl(hash, ROTATE) ^ std::bit_cast<uint32_t>(buffer)) * SEED;
					count -= 4;
					index += 4;
				}
			}

			if (count >= 2) {
				alignas(uint16_t) char buffer[2];
				str.copy(buffer, 2, index);
				hash = (std::rotl(hash, ROTATE) ^ std::bit_cast<uint16_t>(buffer)) * SEED;
				count -= 2;
				index += 2;
			}

			if (count == 1) {
				hash = (std::rotl(hash, ROTATE) ^ std::bit_cast<uint8_t>(str.back())) * SEED;
				count -= 1;
				index += 1;
			}
			
			return hash;
		}
		
		template<std::size_t N>
		static consteval std::size_t get_hash(const char (&str)[N]) { 
			return get_hash(std::string_view{ str });
		}
		
	public:
		template<typename T>
		consteval id(std::type_identity<T> value) : value(get_name<T>().data()), hash(get_hash(get_name<T>())) { }

		template<std::size_t N>
		consteval id(const char (&str)[N]) : value(str), hash(get_hash(str)) { }

		constexpr friend bool operator==(const id& lhs, const id& rhs) { return lhs.value == rhs.value; }
		constexpr friend bool operator!=(const id& lhs, const id& rhs) { return lhs.value != rhs.value; }

		constexpr friend bool operator<=(const id& lhs, const id& rhs) { return lhs.hash <= rhs.hash; }
		constexpr friend bool operator<(const id& lhs, const id& rhs) { return lhs.hash < rhs.hash; }

		constexpr friend bool operator>=(const id& lhs, const id& rhs) { return lhs.hash >= rhs.hash; }
		constexpr friend bool operator>(const id& lhs, const id& rhs) { return lhs.hash > rhs.hash; }

		const char* value;
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