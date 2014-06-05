#include <boost/bind.hpp>

#include <redisclient/redisclient.h>

static const std::string redisKey = "unique-redis-key-example";
static const std::string redisValue = "unique-redis-value";

class Worker
{
public:
    Worker(boost::asio::io_service &ioService, RedisClient &redisClient)
        : ioService(ioService), redisClient(redisClient)
    {}

    void onConnect(const boost::system::error_code &ec);
    void onSet(const RedisValue &value);
    void onGet(const RedisValue &value);
    void stop();

private:
    boost::asio::io_service &ioService;
    RedisClient &redisClient;
};

void Worker::onConnect(const boost::system::error_code &ec)
{
    if( ec )
    {
        std::cerr << "Can't connect to redis: " << ec.message();
    }
    else
    {
        redisClient.command("SET",  redisKey, redisValue,
                            boost::bind(&Worker::onSet, this, _1));
    }
}

void Worker::onSet(const RedisValue &value)
{
    if( value.toString() == "OK" )
    {
        redisClient.command("GET",  redisKey,
                            boost::bind(&Worker::onGet, this, _1));
    }
    else
    {
        std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
    }
}

void Worker::onGet(const RedisValue &value)
{
    if( value.toString() != redisValue )
    {
        std::cerr << "Invalid value from redis: " << value.toString() << std::endl;
    }

    redisClient.command("DEL", redisKey,
                        boost::bind(&boost::asio::io_service::stop, boost::ref(ioService)));
}


int main(int, char **)
{
    const char *address = "127.0.0.1";
    const int port = 6379;

    boost::asio::io_service ioService;
    RedisClient client(ioService);
    Worker worker(ioService, client);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);

    client.asyncConnect(endpoint, boost::bind(&Worker::onConnect, &worker, _1));

    ioService.run();

    return 0;
}
