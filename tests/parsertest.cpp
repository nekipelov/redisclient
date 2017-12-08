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
    RedisValue parse(const char *str)
    {
        BOOST_REQUIRE(parser.parse(str, strlen(str)).second == RedisParser::Completed);
        return parser.result();
    }

    RedisParser::ParseResult parserResult(const char *str)
    {
        return parser.parse(str, strlen(str)).second;
    }

    void parseByPartsTest(const std::string &buf, const RedisValue &expected)
    {
        for(size_t partSize = 1; partSize < buf.size(); ++partSize)
        {
            std::pair<size_t, RedisParser::ParseResult> pair;

            for(size_t i = 0; i < buf.size(); i+= partSize )
            {
                size_t chunkSize = std::min(partSize, buf.size() - i);
                pair = parser.parse(buf.c_str() + i, chunkSize);

                BOOST_REQUIRE(pair.second != RedisParser::Error);
                BOOST_REQUIRE_EQUAL(pair.first, chunkSize);
            }

            BOOST_REQUIRE(pair.second == RedisParser::Completed);
            BOOST_CHECK(parser.result() == expected);
        }
    }

    RedisParser parser;
};

BOOST_FIXTURE_TEST_CASE(test_string_response, ParserFixture)
{
    RedisValue value = parse("+STRING\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isString(), true);
    BOOST_CHECK_EQUAL(value.toString(), "STRING");
}

BOOST_FIXTURE_TEST_CASE(test_error, ParserFixture)
{
    RedisValue value = parse("-Error message\r\n");

    BOOST_CHECK_EQUAL(value.isError(), true);
    BOOST_CHECK_EQUAL(value.isOk(), false);
    BOOST_CHECK_EQUAL(value.toString(), "Error message");
}

BOOST_FIXTURE_TEST_CASE(test_integer_1, ParserFixture)
{
    RedisValue value = parse(":1\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isInt(), true);
    BOOST_CHECK_EQUAL(value.toInt(), 1);
}

BOOST_FIXTURE_TEST_CASE(test_integer_2, ParserFixture)
{
    RedisValue value = parse(":123456789\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isInt(), true);
    BOOST_CHECK_EQUAL(value.toInt(), 123456789);
}

BOOST_FIXTURE_TEST_CASE(test_integer_1_negative, ParserFixture)
{
    RedisValue value = parse(":-1\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isInt(), true);
    BOOST_CHECK_EQUAL(value.toInt(), -1);
}

BOOST_FIXTURE_TEST_CASE(test_integer_2_negative, ParserFixture)
{
    RedisValue value = parse(":-123456789\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isInt(), true);
    BOOST_CHECK_EQUAL(value.toInt(), -123456789);
}

BOOST_FIXTURE_TEST_CASE(test_string_bulk, ParserFixture)
{
    RedisValue value = parse("$6\r\nfoobar\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isByteArray(), true);
    BOOST_CHECK_EQUAL(value.toString(), "foobar");
}

BOOST_FIXTURE_TEST_CASE(test_string_empty_bulk, ParserFixture)
{
    RedisValue value = parse("$0\r\n\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isByteArray(), true);
    BOOST_CHECK_EQUAL(value.toString(), "");
}

BOOST_FIXTURE_TEST_CASE(test_string_null, ParserFixture)
{
    RedisValue value = parse("$-1\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isNull(), true);
}

BOOST_FIXTURE_TEST_CASE(test_string_empty_array_1, ParserFixture)
{
    RedisValue value = parse("*0\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isArray(), true);
    BOOST_CHECK(value.toArray() == std::vector<RedisValue>());
}

BOOST_FIXTURE_TEST_CASE(test_string_empty_array_2, ParserFixture)
{
    RedisValue value = parse("*-1\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isArray(), false);
    BOOST_CHECK_EQUAL(value.isNull(), true);
    BOOST_CHECK(value.toArray() == std::vector<RedisValue>());
}

BOOST_FIXTURE_TEST_CASE(test_array, ParserFixture)
{
    RedisValue value = parse("*2\r\n:1\r\n+TEST\r\n");

    BOOST_CHECK_EQUAL(value.isError(), false);
    BOOST_CHECK_EQUAL(value.isOk(), true);
    BOOST_CHECK_EQUAL(value.isArray(), true);
    BOOST_CHECK(value.toArray() == std::vector<RedisValue>({1, "TEST"}));
}

BOOST_FIXTURE_TEST_CASE(test_recursive_array, ParserFixture)
{
    RedisValue value = parse("*2\r\n*2\r\n:1\r\n:2\r\n*2\r\n:3\r\n:4\r\n");

    BOOST_REQUIRE_EQUAL(value.isArray(), true);
    BOOST_REQUIRE_EQUAL(value.toArray().size(), 2);

    RedisValue subValue0 = value.toArray()[0];
    RedisValue subValue1 = value.toArray()[1];

    BOOST_REQUIRE_EQUAL(subValue0.isArray(), true);
    BOOST_REQUIRE_EQUAL(subValue1.isArray(), true);
    BOOST_REQUIRE_EQUAL(subValue0.toArray().size(), 2);
    BOOST_REQUIRE_EQUAL(subValue1.toArray().size(), 2);

    BOOST_CHECK_EQUAL(subValue0.toArray()[0].isInt(), true);
    BOOST_CHECK_EQUAL(subValue0.toArray()[1].isInt(), true);
    BOOST_CHECK_EQUAL(subValue1.toArray()[0].isInt(), true);
    BOOST_CHECK_EQUAL(subValue1.toArray()[1].isInt(), true);

    BOOST_CHECK_EQUAL(subValue0.toArray()[0].toInt(), 1);
    BOOST_CHECK_EQUAL(subValue0.toArray()[1].toInt(), 2);
    BOOST_CHECK_EQUAL(subValue1.toArray()[0].toInt(), 3);
    BOOST_CHECK_EQUAL(subValue1.toArray()[1].toInt(), 4);
}

BOOST_FIXTURE_TEST_CASE(test_incomplete, ParserFixture)
{
    BOOST_CHECK(parserResult("*1\r\n") == RedisParser::Incompleted);
}

BOOST_FIXTURE_TEST_CASE(test_parser_error, ParserFixture)
{
    BOOST_CHECK(parserResult("*xx\r\n") ==  RedisParser::Error);
}

BOOST_FIXTURE_TEST_CASE(test_parser_by_parts, ParserFixture)
{
    parseByPartsTest("+OK\r\n", "OK");
    parseByPartsTest(":1\r\n", 1);
    parseByPartsTest(":321654987\r\n", 321654987);
    parseByPartsTest("-Error message\r\n", "Error message");

    parseByPartsTest("-ERR unknown command 'foobar'\r\n", "ERR unknown command 'foobar'");
    parseByPartsTest("-WRONGTYPE Operation against a key holding the wrong kind of value\r\n", "WRONGTYPE Operation against a key holding the wrong kind of value");
    parseByPartsTest("$6\r\nfoobar\r\n", "foobar");

    parseByPartsTest("$0\r\n\r\n", "");
    parseByPartsTest("$-1\r\n",  RedisValue());
    parseByPartsTest("*0\r\n", std::vector<RedisValue>());
    parseByPartsTest("*-1\r\n", RedisValue());

    {
        std::vector<RedisValue> array;

        array.push_back("foo");
        array.push_back("bar");

        parseByPartsTest("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", array);
    }

    {
        std::vector<RedisValue> array;

        array.push_back(1);
        array.push_back(2);
        array.push_back(3);

        parseByPartsTest("*3\r\n:1\r\n:2\r\n:3\r\n", array);
    }

    {
        std::vector<RedisValue> array;

        array.push_back(1);
        array.push_back(2);
        array.push_back(3);
        array.push_back(4);
        array.push_back("foobar");

        parseByPartsTest("*5\r\n"
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

        parseByPartsTest("*2\r\n"
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

        parseByPartsTest("*3\r\n"
                     "$3\r\n"
                     "foo\r\n"
                     "$-1\r\n"
                     "$3\r\n"
                     "bar\r\n", array);
    }
}

