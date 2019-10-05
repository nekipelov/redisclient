/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#ifndef REDISCLIENT_REDISCLIENTIMPL_H
#define REDISCLIENT_REDISCLIENTIMPL_H

#include <array>
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <memory>

#include "redis/parser.hpp"
#include "redis/buffer.hpp"
#include "redis/config.hpp"

namespace redis {
	class base_client: public std::enable_shared_from_this<base_client> {
	public:
		enum class State {
			Unconnected,
			Connecting,
			Connected,
			Subscribed,
			Closed
		};

		REDIS_CLIENT_DECL base_client(asio::io_context &ioService);
		REDIS_CLIENT_DECL ~base_client();

		REDIS_CLIENT_DECL void handleAsyncConnect(const asio::error_code &ec, const std::function<void(bool, const std::string &)> &handler);
		REDIS_CLIENT_DECL size_t subscribe(const std::string &command, const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler);
		REDIS_CLIENT_DECL void singleShotSubscribe(const std::string &command, const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler);
		REDIS_CLIENT_DECL void unsubscribe(const std::string &command, size_t handle_id, const std::string &channel, std::function<void(value)> handler);
		REDIS_CLIENT_DECL void close() noexcept;
		REDIS_CLIENT_DECL State getState() const;
		REDIS_CLIENT_DECL static std::vector<char> makeCommand(const std::deque<buffer> &items);
		REDIS_CLIENT_DECL value doSyncCommand(const std::deque<buffer> &buff);
		REDIS_CLIENT_DECL void doAsyncCommand(std::vector<char> buff, std::function<void(value)> handler);

		REDIS_CLIENT_DECL void sendNextCommand();
		REDIS_CLIENT_DECL void processMessage();
		REDIS_CLIENT_DECL void doProcessMessage(value v);
		REDIS_CLIENT_DECL void asyncWrite(const asio::error_code &ec, const size_t);
		REDIS_CLIENT_DECL void asyncRead(const asio::error_code &ec, const size_t);

		REDIS_CLIENT_DECL void onRedisError(const value &);
		REDIS_CLIENT_DECL static void defaulErrorsubscribe_handler(const std::string &s);

		REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const buffer &buf);
		REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const std::string &s);
		REDIS_CLIENT_DECL static void append(std::vector<char> &vec, const char *s);
		REDIS_CLIENT_DECL static void append(std::vector<char> &vec, char c);

		template<size_t size>
		static inline void append(std::vector<char> &vec, const char (&s)[size]);

		template<typename subscribe_handler>
		inline void post(const subscribe_handler &handler);

		asio::io_context::strand strand;
		asio::ip::tcp::socket socket;
		parser redisParser;
		std::array<char, 4096> buf;
		size_t subscribeSeq;

		typedef std::pair<size_t, std::function<void(const std::vector<char> &buf)> > Msgsubscribe_handlerType;
		typedef std::function<void(const std::vector<char> &buf)> SingleShotsubscribe_handlerType;

		typedef std::multimap<std::string, Msgsubscribe_handlerType> Msgsubscribe_handlersMap;
		typedef std::multimap<std::string, SingleShotsubscribe_handlerType> SingleShotsubscribe_handlersMap;

		std::queue<std::function<void(value)>> handlers;
		std::deque<std::vector<char>> dataWrited;
		std::deque<std::vector<char>> dataQueued;
		Msgsubscribe_handlersMap msgsubscribe_handlers;
		SingleShotsubscribe_handlersMap singleShotMsgsubscribe_handlers;

		std::function<void(const std::string &)> errorsubscribe_handler;
		State state;
	};

	template<size_t size>
	void base_client::append(std::vector<char> &vec, const char (&s)[size]) {
		vec.insert(vec.end(), s, s + size);
	}

	template<typename subscribe_handler>
	inline void base_client::post(const subscribe_handler &handler) {
		asio::post(asio::bind_executor(strand, handler));
	}

	inline std::string to_string(base_client::State state) {
		switch(state) {
			case base_client::State::Unconnected:
				return "Unconnected";
				break;
			case base_client::State::Connecting:
				return "Connecting";
				break;
			case base_client::State::Connected:
				return "Connected";
				break;
			case base_client::State::Subscribed:
				return "Subscribed";
				break;
			case base_client::State::Closed:
				return "Closed";
				break;
		}

		return "Invalid";
	}
}

#ifdef REDIS_CLIENT_HEADER_ONLY
	#include "src/base_client.cpp"
#endif

#endif
