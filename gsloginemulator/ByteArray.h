#pragma once

#include <vector>
#include <cstdint>

namespace gamespy {
	class ByteArray : public std::vector<unsigned char> {
	public:
		ByteArray() = default;

		ByteArray(size_type init_size)
			: std::vector<unsigned char>(init_size)
		{

		}

		template<class T>
		ByteArray(T begin, T end)
			: std::vector<unsigned char>(begin, end)
		{

		}

		void append(const std::string& str) {
			insert(end(), str.begin(), str.end());
		}

		void append(std::uint16_t i) {
			push_back((unsigned char)(i));
			push_back((unsigned char)(i >> 8));
		}

		void append(std::uint32_t i) {
			push_back((unsigned char)(i));
			push_back((unsigned char)(i >> 8));
			push_back((unsigned char)(i >> 16));
			push_back((unsigned char)(i >> 24));
		}
	};
}
