#include <string>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string channelName = "unique-redis-channel-name-example";

void subscribeHandler(boost::asio::io_service &ioService, const std::string &msg)
{
    std::cerr << "Message: " << msg << std::endl;

    if( msg == "stop" )
        ioService.stop();
}

void publishHandler(RedisAsyncClient &publisher, const RedisValue &)
{
    publisher.publish(channelName, "First hello", [&](const RedisValue &) {
        publisher.publish(channelName, "Last hello", [&](const RedisValue &) {
            publisher.publish(channelName, "stop");
        });
    });
}

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;
    RedisAsyncClient publisher(ioService);
    RedisAsyncClient subscriber(ioService);


    publisher.asyncConnect(address, port, [&](bool status, const std::string &err)
    {
        if( !status )
        {
            std::cerr << "Can't connect to to redis" << err << std::endl;
        }
        else
        {
            subscriber.asyncConnect(address, port, [&](bool status, const std::string &err)
            {
                if( !status )
                {
                    std::cerr << "Can't connect to to redis" << err << std::endl;
                }
                else
                {
                    subscriber.subscribe(channelName,
                            boost::bind(&subscribeHandler, boost::ref(ioService), _1),
                            boost::bind(&publishHandler, boost::ref(publisher), _1));
                }
            });
        }
    });

    ioService.run();

    return 0;
}
