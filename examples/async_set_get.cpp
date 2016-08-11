#include <string>
#include <iostream>
#include <functional>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string redisKey = "unique-redis-key-example";
static const std::string redisValue = "unique-redis-value";

class Worker
{
public:
    Worker(boost::asio::io_service &ioService, redisclient::RedisAsyncClient &redisClient)
        : ioService(ioService), redisClient(redisClient)
    {}

    void onConnect(bool connected, const std::string &errorMessage);
    void onSet(const redisclient::RedisValue &value);
    void onGet(const redisclient::RedisValue &value);
    void stop();

private:
    boost::asio::io_service &ioService;
    redisclient::RedisAsyncClient &redisClient;
};

void Worker::onConnect(bool connected, const std::string &errorMessage)
{
    if( !connected )
    {
        std::cerr << "Can't connect to redis: " << errorMessage;
    }
    else
    {
        redisClient.command("SET",  {redisKey, redisValue},
                            std::bind(&Worker::onSet, this, std::placeholders::_1));
    }
}

void Worker::onSet(const redisclient::RedisValue &value)
{
    std::cerr << "SET: " << value.toString() << std::endl;
    if( value.toString() == "OK" )
    {
        redisClient.command("GET",  {redisKey},
                            std::bind(&Worker::onGet, this, std::placeholders::_1));
    }
    else
    {
        std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
    }
}

void Worker::onGet(const redisclient::RedisValue &value)
{
    std::cerr << "GET " << value.toString() << std::endl;
    if( value.toString() != redisValue )
    {
        std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
    }

    redisClient.command("DEL", {redisKey},
                        std::bind(&boost::asio::io_service::stop, std::ref(ioService)));
}


int main(int, char **)
{
    const char *address = "127.0.0.1";
    const int port = 6379;

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient client(ioService);
    Worker worker(ioService, client);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);

    client.asyncConnect(endpoint, std::bind(&Worker::onConnect, &worker,
                std::placeholders::_1, std::placeholders::_2));

    ioService.run();

    return 0;
}
