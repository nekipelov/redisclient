#include <string>
#include <vector>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <redisclient/redissyncclient.h>

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    RedisSyncClient redis(ioService);
    std::string errmsg;

    if( !redis.connect(address, port, errmsg) )
    {
        std::cerr << "Can't connect to redis: " << errmsg << std::endl;
        return EXIT_FAILURE;
    }

    const std::string key = "unique-redis-key-example";
    const char rawValue [] = "unique-redis-value";
    const std::string stringValue(rawValue, rawValue + sizeof(rawValue));
    const std::vector<char> vectorValue(rawValue, rawValue + sizeof(rawValue));

    {
        // raw
        redis.command("SET", key, rawValue);
        RedisValue value = redis.command("GET", key);
        
        if( value.toString() != rawValue )
        {
            std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
            return EXIT_FAILURE;
        }
    }

    {
        // std::string
        redis.command("SET", key, stringValue);
        RedisValue value = redis.command("GET", key);
        
        if( value.toString() != stringValue )
        {
            std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
            return EXIT_FAILURE;
        }
    }

    {
        // vector<char>
        redis.command("SET", key, vectorValue);
        RedisValue value = redis.command("GET", key);
        /*
        if( value.toString() != vectorValue )
        {
            std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
            return EXIT_FAILURE;
        }
        * */
    }

    return EXIT_SUCCESS;
}
