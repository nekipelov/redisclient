#ifndef REDIS_ASYNC_CLIENT_HEADER
#define REDIS_ASYNC_CLIENT_HEADER

#include <asio/io_service.hpp>

#include <string>
#include <list>
#include <type_traits>
#include <functional>

#include "redis/base_client.hpp"
#include "redis/value.hpp"
#include "redis/buffer.hpp"
#include "redis/config.hpp"

namespace redis {
	class base_client;

	class async_client {
	public:
		// Subscribe handle.
		struct subscribe_handle {
			size_t id;
			std::string channel;
		};

		typedef base_client::State State;

		REDIS_CLIENT_DECL async_client(asio::io_context &ioService);
		REDIS_CLIENT_DECL ~async_client();

		// Connect to redis server
		REDIS_CLIENT_DECL void connect(const asio::ip::address &address, unsigned short port, std::function<void(bool, const std::string &)> handler);

		// Connect to redis server
		REDIS_CLIENT_DECL void connect(const asio::ip::tcp::endpoint &endpoint, std::function<void(bool, const std::string &)> handler);

		// backward compatibility
		inline void asyncConnect(const asio::ip::address &address, unsigned short port, std::function<void(bool, const std::string &)> handler) {
			connect(address, port, std::move(handler));
		}

		// backward compatibility
		inline void asyncConnect(const asio::ip::tcp::endpoint &endpoint, std::function<void(bool, const std::string &)> handler) {
			connect(endpoint, handler);
		}

		// Return true if is connected to redis.
		// Deprecated: use state() == RedisAsyncClisend::State::Connected.
		REDIS_CLIENT_DECL bool isConnected() const;

		// Return connection state. See base_client::State.
		REDIS_CLIENT_DECL State state() const;

		// disconnect from redis and clear command queue
		REDIS_CLIENT_DECL void disconnect();

		// Set custom error handler.
		REDIS_CLIENT_DECL void installErrorsubscribe_handler(std::function<void(const std::string &)> handler);

		// Execute command on Redis server with the list of arguments.
		REDIS_CLIENT_DECL void command(const std::string &cmd, std::deque<buffer> args, std::function<void(value)> handler = dummysubscribe_handler);

		// Subscribe to channel. subscribe_handler msgsubscribe_handler will be called
		// when someone publish message on channel. Call unsubscribe 
		// to stop the subscription.
		REDIS_CLIENT_DECL subscribe_handle subscribe(const std::string &channelName, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler = &dummysubscribe_handler);


		REDIS_CLIENT_DECL subscribe_handle psubscribe(const std::string &pattern, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler = &dummysubscribe_handler);

		// Unsubscribe
		REDIS_CLIENT_DECL void unsubscribe(const subscribe_handle &handle);
		REDIS_CLIENT_DECL void punsubscribe(const subscribe_handle &handle);

		// Subscribe to channel. subscribe_handler msgsubscribe_handler will be called
		// when someone publish message on channel; it will be 
		// unsubscribed after call.
		REDIS_CLIENT_DECL void singleShotSubscribe(const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler = &dummysubscribe_handler);

		REDIS_CLIENT_DECL void singleShotPSubscribe(const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler = &dummysubscribe_handler);

		// Publish message on channel.
		REDIS_CLIENT_DECL void publish(const std::string &channel, const buffer &msg, std::function<void(value)> handler = &dummysubscribe_handler);
		REDIS_CLIENT_DECL static void dummysubscribe_handler(value) {}

	protected:
		REDIS_CLIENT_DECL bool stateValid() const;

	private:
		std::shared_ptr<base_client> pimpl;
	};
}

#ifdef REDIS_CLIENT_HEADER_ONLY
	#include "src/async_client.cpp"
#endif

#endif
