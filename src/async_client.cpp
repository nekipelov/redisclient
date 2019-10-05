#ifndef REDIS_ASYNC_CLIENT_SOURCE
#define REDIS_ASYNC_CLIENT_SOURCE

#include <memory>
#include <functional>

#include "redis/async_client.hpp"

namespace redis {
	async_client::async_client(asio::io_context &ioService) : pimpl(std::make_shared<base_client>(ioService)) {
		pimpl->errorsubscribe_handler = std::bind(&base_client::defaulErrorsubscribe_handler, std::placeholders::_1);
	}

	async_client::~async_client() {
		pimpl->close();
	}

	void async_client::connect(const ::asio::ip::address &address, unsigned short port, std::function<void(bool, const std::string &)> handler) {
		asio::ip::tcp::endpoint endpoint(address, port);
		connect(endpoint, std::move(handler));
	}

	void async_client::connect(const asio::ip::tcp::endpoint &endpoint, std::function<void(bool, const std::string &)> handler) {
		if(pimpl->state == State::Unconnected || pimpl->state == State::Closed) {
			pimpl->state = State::Connecting;
			pimpl->socket.async_connect(endpoint, std::bind(&base_client::handleAsyncConnect, pimpl, std::placeholders::_1, std::move(handler)));
		} else {
			std::stringstream ss;

			ss << "async_client::connect called on socket with state " << to_string(pimpl->state);
			handler(false, ss.str());
		}
	}

	bool async_client::isConnected() const {
		return pimpl->getState() == State::Connected || pimpl->getState() == State::Subscribed;
	}

	void async_client::disconnect() {
		pimpl->close();
	}

	void async_client::installErrorsubscribe_handler(std::function<void(const std::string &)> handler) {
		pimpl->errorsubscribe_handler = std::move(handler);
	}

	void async_client::command(const std::string &cmd, std::deque<buffer> args, std::function<void(value)> handler) {
		if(stateValid()) {
			args.emplace_front(cmd);

			pimpl->post(std::bind(&base_client::doAsyncCommand, pimpl, std::move(pimpl->makeCommand(args)), std::move(handler)));
		}
	}

	async_client::subscribe_handle async_client::subscribe(const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		auto handleId = pimpl->subscribe("subscribe", channel, msgsubscribe_handler, handler);    
		return { handleId , channel };
	}

	async_client::subscribe_handle async_client::psubscribe(const std::string &pattern, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		auto handleId = pimpl->subscribe("psubscribe", pattern, msgsubscribe_handler, handler);
		return{ handleId , pattern };
	}

	void async_client::unsubscribe(const subscribe_handle &handle) {
		pimpl->unsubscribe("unsubscribe", handle.id, handle.channel, dummysubscribe_handler);
	}

	void async_client::punsubscribe(const subscribe_handle &handle) {
		pimpl->unsubscribe("punsubscribe", handle.id, handle.channel, dummysubscribe_handler);
	}

	void async_client::singleShotSubscribe(const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		pimpl->singleShotSubscribe("subscribe", channel, msgsubscribe_handler, handler);
	}

	void async_client::singleShotPSubscribe(const std::string &pattern, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		pimpl->singleShotSubscribe("psubscribe", pattern, msgsubscribe_handler, handler);
	}

	void async_client::publish(const std::string &channel, const buffer &msg, std::function<void(value)> handler) {
		assert(pimpl->state == State::Connected );
		static const std::string publishStr = "PUBLISH";

		if(pimpl->state == State::Connected) {
			std::deque<buffer> items(3);

			items[0] = publishStr;
			items[1] = channel;
			items[2] = msg;

			pimpl->post(std::bind(&base_client::doAsyncCommand, pimpl, pimpl->makeCommand(items), std::move(handler)));
		} else {
			std::stringstream ss;

			ss << "async_client::command called with invalid state " << to_string(pimpl->state);
			pimpl->errorsubscribe_handler(ss.str());
		}
	}

	async_client::State async_client::state() const {
		return pimpl->getState();
	}

	bool async_client::stateValid() const {
		assert( pimpl->state == State::Connected );

		if(pimpl->state != State::Connected) {
			std::stringstream ss;

			ss << "async_client::command called with invalid state " << to_string(pimpl->state);

			pimpl->errorsubscribe_handler(ss.str());
			return false;
		}

		return true;
	}
}

#endif
