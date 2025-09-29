#pragma once
#include <cstdint>
#include <string_view>

#if defined(__clang__)
#define PF_CMD __PRETTY_FUNCTION__
#define PF_PRE "std::size_t ecs::id::type_hash() [T = "
#define PF_SUF "]"
#elif defined(__GNUC__) && !defined(__clang__)
#define PF_CMD __PRETTY_FUNCTION__
#define PF_PRE "constexpr std::size_t ecs::id::type_hash() [with T = "
#define PF_SUF "]"
#elif defined(_MSC_VER)
#define PF_CMD __FUNCSIG__
#define PF_PRE "struct std::size_t __cdecl ecs::id::type_hash<"
#define PF_SUF ">(void)"
#else
#error "compiler unsupported."
#endif
#define FNV_PRIME_64 0x00000100000001B3
#define FNV_OFFSET_64 0xCBF29CE484222325

namespace ecs {
	struct attrib_id {
	private:
		static constexpr std::size_t str_hash(std::string_view str_view) { 
			auto* str = str_view.data();
			uint64_t hash = FNV_OFFSET_64;
			while(*str) {
				hash ^= static_cast<uint64_t>(*str++);
				hash *= FNV_PRIME_64;
			}
			return hash;
		}
		
		template<typename T>
		static constexpr std::size_t type_hash() {
			std::basic_string_view<char> str_view{ PF_CMD + sizeof(PF_PRE) - 1, sizeof(PF_CMD) + 1 - sizeof(PF_PRE) - sizeof(PF_SUF) - 1 };
			return str_hash(str_view);
		}

		template<typename> friend struct std::hash;
		template<typename T> static constexpr char reserve{};
	public:
		template<typename T>
		consteval attrib_id(std::type_identity<T> val = std::type_identity<T>{}) : val(&reserve<T>), hash(type_hash<T>()) { }
		template<std::size_t N>
		consteval attrib_id(const char (&val)[N]) : val(static_cast<const void*>(val)), hash(str_hash(val)) { }

		constexpr friend bool operator==(const attrib_id& lhs, const attrib_id& rhs) { return lhs.val == rhs.val; }
		constexpr friend bool operator!=(const attrib_id& lhs, const attrib_id& rhs) { return lhs.val != rhs.val; }

		constexpr friend bool operator<=(const attrib_id& lhs, const attrib_id& rhs) { return lhs.hash <= rhs.hash; }
		constexpr friend bool operator<(const attrib_id& lhs, const attrib_id& rhs) { return lhs.hash < rhs.hash; }

		constexpr friend bool operator>=(const attrib_id& lhs, const attrib_id& rhs) { return lhs.hash >= rhs.hash; }
		constexpr friend bool operator>(const attrib_id& lhs, const attrib_id& rhs) { return lhs.hash > rhs.hash; }

	private:
		const void* val;
		const std::size_t hash;
	};
}

#undef FNV_PRIME_64
#undef FNV_OFFSET_64
#undef PF_CMD
#undef PF_PRE
#undef PF_SUF

namespace std {
	template<> 
	struct hash<ecs::attrib_id> {
		constexpr hash() = default;
		constexpr size_t operator()(const ecs::attrib_id& id) const {
			return id.hash;
		}
	};
}