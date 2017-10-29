#include <string>
#include <iostream>
#include <functional>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string channelName = "unique-redis-channel-name-example";


class Client
{
public:
    Client(boost::asio::io_service &ioService)
        : ioService(ioService)
    {}

    void onMessage(const std::vector<char> &buf)
    {
        std::string msg(buf.begin(), buf.end());

        std::cerr << "Message: " << msg << std::endl;

        if( msg == "stop" )
            ioService.stop();
    }

private:
    boost::asio::io_service &ioService;
};

void publishHandler(redisclient::RedisAsyncClient &publisher, const redisclient::RedisValue &)
{
    publisher.publish(channelName, "First hello", [&](const redisclient::RedisValue &) {
        publisher.publish(channelName, "Last hello", [&](const redisclient::RedisValue &) {
            publisher.publish(channelName, "stop");
        });
    });
}

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;
    boost::asio::ip::tcp::endpoint endpoint(address, port);

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient publisher(ioService);
    redisclient::RedisAsyncClient subscriber(ioService);
    Client client(ioService);

    publisher.connect(endpoint, [&](boost::system::error_code ec)
    {
        if(ec)
        {
            std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
        }
        else
        {
            subscriber.connect(endpoint, [&](boost::system::error_code ec)
            {
                if( ec )
                {
                    std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
                }
                else
                {
                    subscriber.subscribe(channelName,
                            std::bind(&Client::onMessage, &client, std::placeholders::_1),
                            std::bind(&publishHandler, std::ref(publisher), std::placeholders::_1));
                }
            });
        }
    });

    ioService.run();

    return 0;
}
