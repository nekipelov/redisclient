#include <iostream>

#include <redis/async_client.hpp>

static const std::string channelName = "unique-redis-channel-name-example";

using namespace redis;

void subscribeHandler(asio::io_service& ioService, const std::vector<char>& buf) {
	std::string msg(buf.begin(), buf.end());
	std::cout << msg << std::endl;

	if (msg == "stop") ioService.stop();
}

void publishHandler(async_client& publisher, const value&) {
	publisher.publish(channelName, "First hello", [&](const value&) {
		publisher.publish(channelName, "Last hello", [&](const value&) {
			publisher.publish(channelName, "stop");
		});
	});
}

int main(int, char**) {
	asio::ip::address address = asio::ip::address::from_string("127.0.0.1");
	const unsigned short port = 6379;

	asio::io_service ioService;
	async_client publisher(ioService);
	async_client subscriber(ioService);

	publisher.connect(address, port, [&](bool status, const std::string& err) {
		if (!status) {
			std::cerr << "Can't connect to to redis" << err << std::endl;
			std::cin.get();
		} else {
			subscriber.connect(address, port, [&](bool status, const std::string& err) {
				if (!status) {

					std::cerr << "Can't connect to to redis" << err << std::endl;
					std::cin.get();
				} else {
					subscriber.subscribe(channelName, std::bind(&subscribeHandler, std::ref(ioService), std::placeholders::_1), std::bind(&publishHandler, std::ref(publisher), std::placeholders::_1));
				}
			});
		}
	});

	ioService.run();
	
	return 0;
}