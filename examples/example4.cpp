#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/crc.hpp>  // for boost::crc_32_type

#include <redisclient/redisclient.h>


#include <boost/asio/version.hpp>

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

    size_t expectedCrc = 0;

    subscriber.subscribe(channelName, [&](const std::string &msg) {
        boost::crc_32_type  crc32;
        crc32.process_bytes(msg.c_str(), msg.size());
        size_t checksum = crc32.checksum(); 

        if( checksum != expectedCrc )
            std::cerr << "fail" << std::endl;
        else
            std::cerr << "ok" << std::endl;
        ioService.stop();
    });

    ioService.poll();
    
    size_t size = 200000;
    std::string msg;


    for(size_t i = 0; i  < size; ++i)
        msg += (boost::format("Hello! Message number %1%") % i).str();

    boost::crc_32_type  crc32;
    crc32.process_bytes(msg.c_str(), msg.size());
    expectedCrc = crc32.checksum(); 

    publisher.publish(channelName, msg, [&](const RedisValue &) {});

    ioService.run();

    return 0;
}
