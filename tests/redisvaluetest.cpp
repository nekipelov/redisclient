#include <stdlib.h>
#include <string.h>
#include <limits>

#include <redisclient/redisvalue.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE test_RedisParser

#include <boost/test/unit_test.hpp>

using namespace redisclient;


BOOST_AUTO_TEST_CASE(test_integer)
{
    RedisValue v(100);

    BOOST_CHECK_EQUAL(v.toInt(), 100);
    BOOST_CHECK_EQUAL(v.isInt(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_integer_pisitive)
{
    RedisValue v(std::numeric_limits<int64_t>::max());

    BOOST_CHECK_EQUAL(v.toInt(), std::numeric_limits<int64_t>::max());
    BOOST_CHECK_EQUAL(v.isInt(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_integer_negative)
{
    RedisValue v(std::numeric_limits<int64_t>::min());

    BOOST_CHECK_EQUAL(v.toInt(), std::numeric_limits<int64_t>::min());
    BOOST_CHECK_EQUAL(v.isInt(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_const_char)
{
    RedisValue v("test string");

    BOOST_CHECK_EQUAL(v.toString(), "test string");
    BOOST_CHECK_EQUAL(v.isString(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_std_string)
{
    RedisValue v(std::string("test string"));

    BOOST_CHECK_EQUAL(v.toString(), "test string");
    BOOST_CHECK_EQUAL(v.isString(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_byte_array)
{
    std::vector<char> byteArray{'t', 'e', 's', 't'};
    RedisValue v(byteArray);

    BOOST_CHECK(v.toByteArray() == byteArray);
    BOOST_CHECK_EQUAL(v.isByteArray(), true);
    BOOST_CHECK_EQUAL(v.isOk(), true);
}

BOOST_AUTO_TEST_CASE(test_error)
{
    std::vector<char> byteArray{'t', 'e', 's', 't'};
    RedisValue v(byteArray, RedisValue::ErrorTag{});

    BOOST_CHECK(v.toByteArray() == byteArray);
    BOOST_CHECK_EQUAL(v.isByteArray(), true);
    BOOST_CHECK_EQUAL(v.isOk(), false);
    BOOST_CHECK_EQUAL(v.isError(), true);
}

