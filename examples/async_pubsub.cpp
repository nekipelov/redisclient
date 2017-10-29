#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <redisclient/redisasyncclient.h>

using namespace redisclient;

static const std::string channelName = "unique-redis-channel-name-example";
static const boost::posix_time::seconds timeout(1);

class Client
{
public:
    Client(boost::asio::io_service &ioService,
           const boost::asio::ip::address &address,
           unsigned short port)
        : ioService(ioService), publishTimer(ioService),
          connectSubscriberTimer(ioService), connectPublisherTimer(ioService),
          address(address), port(port),
          publisher(ioService), subscriber(ioService)
    {
        publisher.installErrorHandler(std::bind(&Client::connectPublisher, this));
        subscriber.installErrorHandler(std::bind(&Client::connectSubscriber, this));
    }

    void publish(const std::string &str)
    {
        publisher.publish(channelName, str);
    }

    void start()
    {
        connectPublisher();
        connectSubscriber();
    }

protected:
    void errorPubProxy(const std::string &)
    {
        publishTimer.cancel();
        connectPublisher();
    }

    void errorSubProxy(const std::string &)
    {
        connectSubscriber();
    }

    void connectPublisher()
    {
        std::cerr << "connectPublisher\n";

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "disconnectPublisher\n";

            publisher.disconnect();
            publishTimer.cancel();
        }

        boost::asio::ip::tcp::endpoint endpoint(address, port);
        publisher.connect(endpoint,
                          std::bind(&Client::onPublisherConnected, this, std::placeholders::_1));
    }

    void connectSubscriber()
    {
        std::cerr << "connectSubscriber\n";

        if( subscriber.state() == RedisAsyncClient::State::Connected ||
                subscriber.state() == RedisAsyncClient::State::Subscribed )
        {
            std::cerr << "disconnectSubscriber\n";
            subscriber.disconnect();
        }

        boost::asio::ip::tcp::endpoint endpoint(address, port);
        subscriber.connect(endpoint,
                           std::bind(&Client::onSubscriberConnected, this, std::placeholders::_1));
    }

    void callLater(boost::asio::deadline_timer &timer,
                   void(Client::*callback)())
    {
        std::cerr << "callLater\n";
        timer.expires_from_now(timeout);
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if( !ec )
            {
                (this->*callback)();
            }
        });
    }

    void onPublishTimeout()
    {
        static size_t counter = 0;
        std::string msg = str(boost::format("message %1%")  % counter++);

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "pub " << msg << "\n";
            publish(msg);
        }

        callLater(publishTimer, &Client::onPublishTimeout);
    }

    void onPublisherConnected(boost::system::error_code ec)
    {
        if(ec)
        {
            std::cerr << "onPublisherConnected: can't connect to redis: " << ec.message() << "\n";
            callLater(connectPublisherTimer, &Client::connectPublisher);
        }
        else
        {
            std::cerr << "onPublisherConnected ok\n";

            callLater(publishTimer, &Client::onPublishTimeout);
        }
    }

    void onSubscriberConnected(boost::system::error_code ec)
    {
        if( ec )
        {
            std::cerr << "onSubscriberConnected: can't connect to redis: " << ec.message() << "\n";
            callLater(connectSubscriberTimer, &Client::connectSubscriber);
        }
        else
        {
            std::cerr << "onSubscriberConnected ok\n";
            subscriber.subscribe(channelName,
                                 std::bind(&Client::onMessage, this, std::placeholders::_1));
            subscriber.psubscribe("*",
                                 std::bind(&Client::onMessage, this, std::placeholders::_1));
        }
    }

    void onMessage(const std::vector<char> &buf)
    {
        std::string s(buf.begin(), buf.end());
        std::cout << "onMessage: " << s << "\n";
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer publishTimer;
    boost::asio::deadline_timer connectSubscriberTimer;
    boost::asio::deadline_timer connectPublisherTimer;
    const boost::asio::ip::address address;
    const unsigned short port;

    RedisAsyncClient publisher;
    RedisAsyncClient subscriber;
};

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;

    Client client(ioService, address, port);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}
