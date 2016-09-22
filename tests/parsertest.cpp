#include <stdlib.h>
#include <string.h>

#include <redisclient/redisparser.h>
#include <redisclient/redisvalue.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE test_RedisParser

#include <boost/test/unit_test.hpp>

using namespace redisclient;

class ParserFixture
{
public:
    RedisValue parse(const char *str) {
        if( parser.parse(str, strlen(str)).second == RedisParser::Completed)
            return parser.result();
        else
            return RedisValue();
    }

    RedisParser::ParseResult parserResult(const char *str) {
        return parser.parse(str, strlen(str)).second;
    }

    RedisParser parser;
};

void test(RedisParser &parser, const std::string &buf, const RedisValue &expected)
{
    for(size_t partSize = 1; partSize < buf.size(); ++partSize)
    {
        std::pair<size_t, RedisParser::ParseResult> pair;

        for(size_t i = 0; i < buf.size(); i+= partSize )
        {
            size_t chunkSize = std::min(partSize, buf.size() - i);
            pair = parser.parse(buf.c_str() + i, chunkSize);

            BOOST_REQUIRE(pair.second != RedisParser::Error);
            BOOST_REQUIRE(pair.first == chunkSize);
        }

        BOOST_REQUIRE(pair.second == RedisParser::Completed);
        BOOST_VERIFY(parser.result() == expected);


//        BOOST_VERIFY(parser.valueStack.empty());
//        BOOST_VERIFY(parser.arrayStack.empty());
    }
}


BOOST_AUTO_TEST_CASE(test_parser)
{
    RedisParser parser;

    test(parser, "+OK\r\n", "OK");
    test(parser, ":1\r\n", 1);
    test(parser, ":321654987\r\n", 321654987);
    test(parser, "-Error message\r\n", "Error message");

    test(parser, "-ERR unknown command 'foobar'\r\n", "ERR unknown command 'foobar'");
    test(parser, "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n", "WRONGTYPE Operation against a key holding the wrong kind of value");
    test(parser, "$6\r\nfoobar\r\n", "foobar");
    test(parser, "$0\r\n\r\n", "");
    test(parser, "$-1\r\n",  RedisValue());
    test(parser, "*0\r\n", std::vector<RedisValue>());
    test(parser, "*-1\r\n", std::vector<RedisValue>());

    {
        std::vector<RedisValue> array;

        array.push_back("foo");
        array.push_back("bar");

        test(parser, "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", array);
    }

    {
        std::vector<RedisValue> array;

        array.push_back(1);
        array.push_back(2);
        array.push_back(3);

        test(parser, "*3\r\n:1\r\n:2\r\n:3\r\n", array);
    }

    {
        std::vector<RedisValue> array;

        array.push_back(1);
        array.push_back(2);
        array.push_back(3);
        array.push_back(4);
        array.push_back("foobar");

        test(parser, "*5\r\n"
                     ":1\r\n"
                     ":2\r\n"
                     ":3\r\n"
                     ":4\r\n"
                     "$6\r\n"
                     "foobar\r\n", array);
    }

    {
        std::vector<RedisValue> array;
        std::vector<RedisValue> sub1;
        std::vector<RedisValue> sub2;

        sub1.push_back(1);
        sub1.push_back(2);
        sub1.push_back(3);

        sub2.push_back("foo");
        sub2.push_back("bar");

        array.push_back(sub1);
        array.push_back(sub2);

        test(parser, "*2\r\n"
                     "*3\r\n"
                     ":1\r\n"
                     ":2\r\n"
                     ":3\r\n"
                     "*2\r\n"
                     "+foo\r\n"
                     "-bar\r\n", array);
    }

    {
        std::vector<RedisValue> array;

        array.push_back("foo");
        array.push_back(RedisValue());
        array.push_back("bar");

        test(parser, "*3\r\n"
                     "$3\r\n"
                     "foo\r\n"
                     "$-1\r\n"
                     "$3\r\n"
                     "bar\r\n", array);
    }
}

BOOST_FIXTURE_TEST_CASE(test_invalid_response, ParserFixture)
{
    BOOST_CHECK_EQUAL(parserResult("*xx\r\n"),  RedisParser::Error);
}

BOOST_FIXTURE_TEST_CASE(test_incomplete, ParserFixture)
{
    BOOST_CHECK_EQUAL(parserResult("*1\r\n"), RedisParser::Incompleted);
}

BOOST_FIXTURE_TEST_CASE(test_error, ParserFixture)
{
    RedisValue value = parse("-Error message\r\n");

    BOOST_CHECK_EQUAL(value.toString(), "Error message");
    BOOST_CHECK_EQUAL(value.isError(), true);
    BOOST_CHECK_EQUAL(value.isOk(), false);
}

