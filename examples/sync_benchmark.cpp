#include <string>
#include <iostream>
#include <regex>
#include <memory>

#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include <redisclient/redissyncclient.h>
#include <redisclient/redisasyncclient.h>


struct Config
{
    std::string address;
    uint16_t port;
};

class Benchmark
{
public:
    Benchmark(boost::asio::io_service &ioService, const Config &config)
        : ioService(ioService), config(config)
    {
    }

    void run()
    {
        boost::asio::ip::tcp::endpoint endpoint(
                boost::asio::ip::address::from_string(config.address), config.port);


        redisclient::RedisSyncClient redisClient(ioService);
        boost::system::error_code ec;

        redisClient.connect(endpoint, ec);

        if (ec)
        {
            std::cerr << "Can't connect to " << config.address << ":" << config.port
                << ": " << ec.message() << "\n";
            exit(-1);
        }
        size_t i = 0;

        for(;;)
        {
            std::string key = str(boost::format("key %1%") % ++i);

            if (redisClient.command("SET", {key, key}) != "OK")
                abort();
            if (redisClient.command("GET", {key}) != key)
                abort();
            if (redisClient.command("DEL", {key}) != 1)
                abort();

            printOPS(redisClient);
        }
    }

private:
    void printOPS(redisclient::RedisSyncClient &redisClient)
    {
        static time_t lastTime = time(0);
        const time_t timeout = 1; // 1 sec;

        if (time(0) >= lastTime + timeout)
        {
            redisclient::RedisValue stats = redisClient.command("info", {"stats"});
            lastTime = time(0);

            static const std::string key("instantaneous_ops_per_sec");
            static const std::regex expression("instantaneous_ops_per_sec:(\\d+)");

            std::smatch matches;
            std::string s = stats.toString();

            if( std::regex_search(s, matches, expression) ) {
                unsigned long ops = std::stoul(matches[1]);

                std::cout << "redis server ops: " << ops << std::endl;
            }
        }
    }

private:
    boost::asio::io_service &ioService;
    Config config;
};

int main(int argc, char **argv)
{
    namespace po = boost::program_options;

    Config config;

    po::options_description description("Options");
    description.add_options()
        ("help", "produce help message")
        ("address", po::value(&config.address)->default_value("127.0.0.1"),
             "redis server ip address")
        ("port", po::value(&config.port)->default_value(6379), "redis server port")
    ;

    po::variables_map vm;

    try
    {
        po::store(po::parse_command_line(argc, argv, description), vm);
        po::notify(vm);
    }
    catch(const po::error &e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    if( vm.count("help") )
    {
        std::cout << description << "\n";
        return EXIT_SUCCESS;
    }

    boost::asio::io_service ioService;
    Benchmark benchmark(ioService, config);

    benchmark.run();

    return 0;
}

