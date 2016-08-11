#include <string>
#include <iostream>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string redisKey = "unique-redis-key-example";
static const std::string redisValue = "unique-redis-value";

void handleConnected(boost::asio::io_service &ioService, redisclient::RedisAsyncClient &redis,
        bool ok, const std::string &errmsg)
{
    if( ok )
    {
        redis.command("SET", {redisKey, redisValue}, [&](const redisclient::RedisValue &v) {
            std::cerr << "SET: " << v.toString() << std::endl;

            redis.command("GET", {redisKey}, [&](const redisclient::RedisValue &v) {
                std::cerr << "GET: " << v.toString() << std::endl;

                redis.command("DEL", {redisKey}, [&](const redisclient::RedisValue &) {
                    ioService.stop();
                });
            });
        });
    }
    else
    {
        std::cerr << "Can't connect to redis: " << errmsg << std::endl;
    }
}

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient redis(ioService);

    redis.asyncConnect(address, port,
            std::bind(&handleConnected, std::ref(ioService), std::ref(redis),
                std::placeholders::_1, std::placeholders::_2));

    ioService.run();

    return 0;
}
