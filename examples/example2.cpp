#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <redisclient/redisclient.h>

static const std::string redisKey = "unique-redis-key-example";
static const std::string redisValue = "unique-redis-value";

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    RedisClient redis(ioService);

    if( !redis.connect(address, port) )
    {
        std::cerr << "Can't connecto to redis" << std::endl;
        return EXIT_FAILURE;
    }

    redis.command("SET", redisKey, redisValue, [&](const RedisValue &v) {
        std::cerr << "SET: " << v.toString() << std::endl;

        redis.command("GET", redisKey, [&](const RedisValue &v) {
            std::cerr << "GET: " << v.toString() << std::endl;

            redis.command("DEL", redisKey, [&](const RedisValue &) {
                ioService.stop();
            });
        });
    });

    ioService.run();

    return 0;
}
