#include <string>
#include <iostream>
#include <functional>
#include <boost/asio/ip/address.hpp>

#include <redisclient/redisasyncclient.h>

static const std::string redisKey = "unique-redis-key-example";

class Worker
{
public:
    Worker(boost::asio::io_service &ioService)
        : ioService(ioService), redisClient(ioService), timer(ioService),
        timeout(boost::posix_time::seconds(3))
    {
    }

    void run();
    void stop();

protected:
    void onConnect(boost::system::error_code ec);
    void onGet(const redisclient::RedisValue &value);
    void get();

    void onTimeout(const boost::system::error_code &ec);

private:
    boost::asio::io_service &ioService;
    redisclient::RedisAsyncClient redisClient;
    boost::asio::deadline_timer timer;
    boost::posix_time::time_duration timeout;
};

void Worker::run()
{
    const char *address = "127.0.0.1";
    const int port = 6379;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);

    timer.expires_from_now(timeout);
    timer.async_wait(std::bind(&Worker::onTimeout, this, std::placeholders::_1));

    redisClient.connect(endpoint, std::bind(&Worker::onConnect, this,
                std::placeholders::_1));
}

void Worker::onConnect(boost::system::error_code ec)
{
    if (ec)
    {
        if (ec != boost::asio::error::operation_aborted)
        {
            timer.cancel();
            std::cerr << "Can't connect to redis: " << ec.message() << "\n";
        }
    }
    else
    {
        std::cerr << "connected\n";
        get();
    }
}

void Worker::onGet(const redisclient::RedisValue &value)
{
    timer.cancel();
    std::cerr << "GET " << value.toString() << std::endl;
    sleep(1);

    get();
}

void Worker::get()
{
    timer.expires_from_now(timeout);
    timer.async_wait(std::bind(&Worker::onTimeout, this, std::placeholders::_1));

    redisClient.command("GET",  {redisKey},
            std::bind(&Worker::onGet, this, std::placeholders::_1));
}


void Worker::onTimeout(const boost::system::error_code &ec)
{
    if (!ec)
    {
        std::cerr << "timeout!\n";
        boost::system::error_code ignore_ec;

        redisClient.disconnect();
        // try again
        run();
    }
}

int main(int, char **)
{
    boost::asio::io_service ioService;
    Worker worker(ioService);

    worker.run();

    ioService.run();

    return 0;
}
