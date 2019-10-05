#ifndef REDIS_BUFFER_HEADER
#define REDIS_BUFFER_HEADER

#include <string>
#include <list>
#include <vector>
#include <string.h>

#include "redis/config.hpp"

namespace redis {
	class buffer {
	public:
		inline buffer();
		inline buffer(const char *ptr, size_t dataSize);
		inline buffer(const char *s);
		inline buffer(const std::string &s);
		inline buffer(const std::vector<char> &buf);

		inline size_t size() const;
		inline const char *data() const;

	private:
		const char *ptr_;
		size_t size_;
	};

	buffer::buffer() : ptr_(NULL), size_(0) {
	}

	buffer::buffer(const char *ptr, size_t dataSize) : ptr_(ptr), size_(dataSize) {
	}

	buffer::buffer(const char *s) : ptr_(s), size_(s == NULL ? 0 : strlen(s)) {
	}

	buffer::buffer(const std::string &s) : ptr_(s.c_str()), size_(s.length()) {
	}

	buffer::buffer(const std::vector<char> &buf) : ptr_(buf.empty() ? NULL : &buf[0]), size_(buf.size()) {
	}

	size_t buffer::size() const {
		return size_;
	}

	const char *buffer::data() const {
		return ptr_;
	}
}

#endif

