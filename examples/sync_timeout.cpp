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
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    boost::asio::io_service ioService;
    redisclient::RedisSyncClient redisClient(ioService);
    boost::system::error_code ec;

    redisClient.setConnectTimeout(boost::posix_time::seconds(3))
        .setCommandTimeout(boost::posix_time::seconds(3));
    redisClient.connect(endpoint, ec);

    if (ec)
    {
        std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
        return EXIT_FAILURE;
    }

    while(1)
    {
        redisclient::RedisValue result = redisClient.command("GET", {"key"}, ec);

        if (ec)
        {
            std::cerr << "Network error: " << ec.message() << "\n";
            break;
        }
        else if (result.isError())
        {
            std::cerr << "GET error: " << result.toString() << "\n";
            return EXIT_FAILURE;
        }
        else
        {
            std::cout << "GET: " << result.toString() << "\n";
        }

        sleep(1);
    }

    return EXIT_FAILURE;
}
