#ifndef REDIS_SYNC_CLIENT_HEADER
#define REDIS_SYNC_CLIENT_HEADER

#include <asio/io_service.hpp>

#include <string>
#include <list>
#include <functional>

#include "redis/base_client.hpp"
#include "redis/buffer.hpp"
#include "redis/value.hpp"
#include "redis/config.hpp"

namespace redis {
	class base_client;

	class sync_client {
	public:
		sync_client(const sync_client&) = delete;
		typedef base_client::State State;

		REDIS_CLIENT_DECL sync_client(asio::io_context &ioService);
		REDIS_CLIENT_DECL ~sync_client();

		// Connect to redis server
		REDIS_CLIENT_DECL bool connect(const asio::ip::tcp::endpoint &endpoint, std::string &errmsg);

		// Connect to redis server
		REDIS_CLIENT_DECL bool connect(const asio::ip::address &address, unsigned short port, std::string &errmsg);

		// Set custom error handler.
		REDIS_CLIENT_DECL void installErrorsubscribe_handler(std::function<void(const std::string &)> handler);

		// Execute command on Redis server with the list of arguments.
		REDIS_CLIENT_DECL value command(const std::string &cmd, std::deque<buffer> args);

		REDIS_CLIENT_DECL value command(const std::string &cmd);

		// Return connection state. See base_client::State.
		REDIS_CLIENT_DECL State state() const;

	protected:
		REDIS_CLIENT_DECL bool stateValid() const;

	private:
		std::shared_ptr<base_client> pimpl;
	};
}

#ifdef REDIS_CLIENT_HEADER_ONLY
	#include "src/sync_client.cpp"
#endif

#endif
