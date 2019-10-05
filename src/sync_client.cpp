#ifndef REDIS_SYNC_CLIENT_SOURCE
#define REDIS_SYNC_CLIENT_SOURCE

#include <memory>
#include <functional>

#include "redis/sync_client.hpp"

namespace redis {
	sync_client::sync_client(asio::io_context &ioService) : pimpl(std::make_shared<base_client>(ioService)) {
		pimpl->errorsubscribe_handler = std::bind(&base_client::defaulErrorsubscribe_handler, std::placeholders::_1);
	}

	sync_client::~sync_client() {
		pimpl->close();
	}

	bool sync_client::connect(const asio::ip::tcp::endpoint &endpoint, std::string &errmsg) {
		asio::error_code ec;
		pimpl->socket.open(endpoint.protocol(), ec);

		if(!ec) {
			pimpl->socket.set_option(asio::ip::tcp::no_delay(true), ec);

			if(!ec) {
				pimpl->socket.connect(endpoint, ec);
			}
		}

		if(!ec) {
			pimpl->state = State::Connected;
			return true;
		} else {
			errmsg = ec.message();
			return false;
		}
	}

	bool sync_client::connect(const asio::ip::address &address, unsigned short port, std::string &errmsg) {
		asio::ip::tcp::endpoint endpoint(address, port);
		return connect(endpoint, errmsg);
	}

	void sync_client::installErrorsubscribe_handler(std::function<void(const std::string &)> handler) {
		pimpl->errorsubscribe_handler = std::move(handler);
	}

	value sync_client::command(const std::string &cmd, std::deque<buffer> args) {
		if(stateValid()) {
			args.emplace_front(cmd);

			return pimpl->doSyncCommand(args);
		} else {
			return value();
		}
	}

	value sync_client::command(const std::string &cmd) {
		if (stateValid()) {
			std::deque<buffer> args;
			args.emplace_front(cmd);
			return pimpl->doSyncCommand(args);
		} else {
			return value();
		}
	}

	sync_client::State sync_client::state() const {
		return pimpl->getState();
	}

	bool sync_client::stateValid() const {
		assert( state() == State::Connected );

		if(state() != State::Connected) {
			std::stringstream ss;
			ss << "RedisClient::command called with invalid state " << to_string(state());

			pimpl->errorsubscribe_handler(ss.str());
			return false;
		}

		return true;
	}
}

#endif
