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
    const char value [] = "unique-redis-value";

    redis.command("SET", key, value);
    RedisValue result = redis.command("GET", key);

    std::cout << "SET " << key << ": " << value << "\n";
    std::cout << "GET " << key << ": " << result.toString() << "\n";
    
    if( result.toString() != value )
    {
        std::cerr << "Invalid value from redis: " << result.toString() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
