#include <string>
#include <iostream>
#include <regex>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


#include <redisclient/redisasyncclient.h>

static const size_t workersCount = 10;

class Worker
{
public:
    Worker(boost::asio::io_service &ioService, redisclient::RedisAsyncClient &redisClient)
        : ioService(ioService), timer(ioService), redisClient(redisClient),
        timeout(boost::posix_time::seconds(1))
    {
    }

    void onConnect(bool connected, const std::string &errorMessage)
    {
        if( !connected )
        {
            std::cerr << "Can't connect to redis: " << errorMessage;
        }
        else
        {
            for(size_t i = 0; i < workersCount; ++i) {
                work();
            }

            resetTimeout();
        }
    }

    void work()
    {
        static int i = 0;
        std::string key = str(boost::format("key %1%") % ++i);

        redisClient.command("SET", {key, key}, [key, this](const redisclient::RedisValue &) {
            redisClient.command("GET", {key}, [key, this](const redisclient::RedisValue &result) {
                assert(result.toString() == key);
                (void)result; // fix unused warning

                redisClient.command("DEL", {key}, [key, this](const redisclient::RedisValue &) {
                    work();
                });
            });
        });
    }

protected:
    void onTimeout(const boost::system::error_code &errorCode) {
        if( errorCode != boost::asio::error::operation_aborted )
        {
            resetTimeout();
            redisClient.command("info", {"stats"},
                    std::bind(&Worker::parseStats, this, std::placeholders::_1));
        }
    }

    void resetTimeout()
    {
        timer.expires_from_now(timeout);
        timer.async_wait(std::bind(&Worker::onTimeout, this, std::placeholders::_1));
    }

    void parseStats(const redisclient::RedisValue &stats)
    {
        std::string s = stats.toString();
        static const std::string key("instantaneous_ops_per_sec");

        static std::regex expression("instantaneous_ops_per_sec:(\\d+)");
        std::smatch matches;

        if( std::regex_search(s, matches, expression) ) {
            unsigned long rps = std::stoul(matches[1]);

            std::cout << "redis server rps: " << rps << std::endl;
        }
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer timer;
    redisclient::RedisAsyncClient &redisClient;
    const boost::posix_time::time_duration timeout;
};


int main(int, char **)
{
    const char *address = "127.0.0.1";
    const int port = 6379;

    boost::asio::io_service ioService;
    redisclient::RedisAsyncClient client(ioService);
    Worker worker(ioService, client);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(address), port);

    client.asyncConnect(endpoint, std::bind(&Worker::onConnect, &worker, std::placeholders::_1, std::placeholders::_2));

    ioService.run();

    return 0;
}

