#ifndef REDIS_PARSER_HEADER
#define REDIS_PARSER_HEADER

#include <stack>
#include <vector>
#include <utility>
#include <algorithm>

#include "redis/value.hpp"
#include "redis/config.hpp"

namespace redis {
	class parser {
	public:
		REDIS_CLIENT_DECL parser();

		enum ParseResult {
			Completed,
			Incompleted,
			Error,
		};

		REDIS_CLIENT_DECL std::pair<size_t, ParseResult> parse(const char *ptr, size_t size);
		REDIS_CLIENT_DECL value result();

	protected:
		REDIS_CLIENT_DECL std::pair<size_t, ParseResult> parseChunk(const char *ptr, size_t size);
		REDIS_CLIENT_DECL std::pair<size_t, ParseResult> parseArray(const char *ptr, size_t size);

		inline bool isChar(int c) {
			return c >= 0 && c <= 127;
		}

		inline bool isControl(int c) {
			return (c >= 0 && c <= 31) || (c == 127);
		}

		REDIS_CLIENT_DECL long int bufToLong(const char *str, size_t size);

	private:
		enum State {
			Start = 0,

			String = 1,
			StringLF = 2,

			ErrorString = 3,
			ErrorLF = 4,

			Integer = 5,
			IntegerLF = 6,

			BulkSize = 7,
			BulkSizeLF = 8,
			Bulk = 9,
			BulkCR = 10,
			BulkLF = 11,

			ArraySize = 12,
			ArraySizeLF = 13,

		} state;

		long int bulkSize;
		std::vector<char> buf;
		std::stack<long int> arrayStack;
		std::stack<value> valueStack;

		static const char stringReply = '+';
		static const char errorReply = '-';
		static const char integerReply = ':';
		static const char bulkReply = '$';
		static const char arrayReply = '*';
	};

}

#ifdef REDIS_CLIENT_HEADER_ONLY
	#include "src/parser.cpp"
#endif

#endif
