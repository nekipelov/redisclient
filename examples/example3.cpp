#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisclient.h>

static const std::string channelName = "unique-redis-channel-name-example";

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    RedisClient publisher(ioService);
    RedisClient subscriber(ioService);

    if( !publisher.connect(address, port) || !subscriber.connect(address, port) )
    {
        std::cerr << "Can't connecto to redis" << std::endl;
        return EXIT_FAILURE;
    }

    subscriber.subscribe(channelName, [&](const std::string &msg) {
        std::cerr << "Message: " << msg << std::endl;

        if( msg == "stop" )
            ioService.stop();
    });

    publisher.publish(channelName, "First hello", [&](const RedisValue &) {
        publisher.publish(channelName, "Last hello", [&](const RedisValue &) {
            publisher.publish(channelName, "stop");
        });
    });

    ioService.run();

    return 0;
}
