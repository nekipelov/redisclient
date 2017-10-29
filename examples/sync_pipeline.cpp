#include <string>
#include <vector>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <redisclient/pipeline.h>
#include <redisclient/redissyncclient.h>

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    boost::asio::io_service ioService;
    redisclient::RedisSyncClient redis(ioService);
    boost::system::error_code ec;

    redis.connect(endpoint, ec);

    if (ec)
    {
        std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
        return EXIT_FAILURE;
    }

    redisclient::Pipeline pipeline = redis.pipelined().command("SET", {"key", "value"})
        .command("INCR", {"counter"})
        .command("INCR", {"counter"})
        .command("DECR", {"counter"})
        .command("GET", {"counter"})
        .command("DEL", {"counter"})
        .command("DEL", {"key"});
    redisclient::RedisValue result = pipeline.finish();

    if (result.isOk())
    {
        for(const auto &i: result.toArray())
        {
            std::cout << "Result: " << i.inspect() << "\n";
        }

        return EXIT_SUCCESS;
    }
    else
    {
        std::cerr << "GET error: " << result.toString() << "\n";
        return EXIT_FAILURE;
    }
}
