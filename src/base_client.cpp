#ifndef REDIS_BASE_CLIENT_SOURCE
#define REDIS_BASE_CLIENT_SOURCE

#include <asio/write.hpp>
#include <asio/bind_executor.hpp>

#include <algorithm>

#include "redis/base_client.hpp"

namespace redis {
	base_client::base_client(asio::io_context &ioService) : strand(ioService), socket(ioService), subscribeSeq(0), state(State::Unconnected) {
	}

	base_client::~base_client() {
		close();
	}

	void base_client::close() noexcept {
		if(state != State::Closed) {
			asio::error_code ignored_ec;

			msgsubscribe_handlers.clear();

			socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
			socket.close(ignored_ec);

			state = State::Closed;
		}
	}

	base_client::State base_client::getState() const {
		return state;
	}

	void base_client::processMessage() {
		socket.async_read_some(asio::buffer(buf), std::bind(&base_client::asyncRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}

	void base_client::doProcessMessage(value v) {
		if( state == State::Subscribed ) {
			std::vector<value> result = v.toArray();
			auto resultSize = result.size();

			if( resultSize >= 3 ) {
				const value &command   = result[0];
				const value &queueName = result[(resultSize == 3)?1:2];
				const value &v     = result[(resultSize == 3)?2:3];
				const value &pattern   = (resultSize == 4) ? result[1] : "";

				std::string cmd = command.toString();

				if(cmd == "message" || cmd == "pmessage") {
					SingleShotsubscribe_handlersMap::iterator it = singleShotMsgsubscribe_handlers.find(queueName.toString());

					if(it != singleShotMsgsubscribe_handlers.end()) {
						asio::post(std::bind(it->second, v.toByteArray()));
						singleShotMsgsubscribe_handlers.erase(it);
					}

					std::pair<Msgsubscribe_handlersMap::iterator, Msgsubscribe_handlersMap::iterator> pair = msgsubscribe_handlers.equal_range(queueName.toString());

					for(Msgsubscribe_handlersMap::iterator handlerIt = pair.first; handlerIt != pair.second; ++handlerIt) {
						asio::post(asio::bind_executor(strand, std::bind(handlerIt->second.second, v.toByteArray())));
					}
				}else if( handlers.empty() == false && (cmd == "subscribe" || cmd == "unsubscribe" || cmd == "psubscribe" || cmd == "punsubscribe")) {
					handlers.front()(std::move(v));
					handlers.pop();
				} else {
					std::stringstream ss;

					ss << "[RedisClient] invalid command: " << command.toString();

					errorsubscribe_handler(ss.str());
					return;
				}
			} else {
				errorsubscribe_handler("[RedisClient] Protocol error");
				return;
			}
		} else {
			if( handlers.empty() == false ) {
				handlers.front()(std::move(v));
				handlers.pop();
			} else {
				std::stringstream ss;
				ss << "[RedisClient] unexpected message: " <<  v.inspect();

				errorsubscribe_handler(ss.str());
				return;
			}
		}
	}

	void base_client::asyncWrite(const asio::error_code &ec, size_t) {
		dataWrited.clear();

		if(ec) {
			errorsubscribe_handler(ec.message());
			return;
		}

		if(dataQueued.empty() == false) {
			std::vector<asio::const_buffer> buffers(dataQueued.size());

			for(size_t i = 0; i < dataQueued.size(); ++i) {
				buffers[i] = ::asio::buffer(dataQueued[i]);
			}

			std::swap(dataQueued, dataWrited);

			asio::async_write(socket, buffers, std::bind(&base_client::asyncWrite, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
		}
	}

	void base_client::handleAsyncConnect(const asio::error_code &ec, const std::function<void(bool, const std::string &)> &handler) {
		if(!ec) {
			socket.set_option(asio::ip::tcp::no_delay(true));
			state = State::Connected;
			handler(true, std::string());
			processMessage();
		} else {
			state = State::Unconnected;
			handler(false, ec.message());
		}
	}

	std::vector<char> base_client::makeCommand(const std::deque<buffer> &items) {
		static const char crlf[] = {'\r', '\n'};
		std::vector<char> result;
		append(result, '*');
		append(result, std::to_string(items.size()));
		append<>(result, crlf);

		for(const buffer &item: items) {
			append(result, '$');
			append(result, std::to_string(item.size()));
			append<>(result, crlf);
			append(result, item);
			append<>(result, crlf);
		}

		return result;
	}

	value base_client::doSyncCommand(const std::deque<buffer> &buff) {
		asio::error_code ec;
		{
			std::vector<char> data = makeCommand(buff);
			asio::write(socket, asio::buffer(data), asio::transfer_all(), ec);
		}

		if(ec) {
			errorsubscribe_handler(ec.message());
			return value();
		} else {
			std::array<char, 4096> inbuff;

			for(;;) {
				size_t size = socket.read_some(asio::buffer(inbuff));

				for(size_t pos = 0; pos < size;) {
					std::pair<size_t, parser::ParseResult> result = redisParser.parse(inbuff.data() + pos, size - pos);

					if(result.second == parser::Completed) {
						return redisParser.result();
					} else if(result.second == parser::Incompleted) {
						pos += result.first;
						continue;
					} else {
						errorsubscribe_handler("[RedisClient] Parser error");
						return value();
					}
				}
			}
		}
	}

	void base_client::doAsyncCommand(std::vector<char> buff, std::function<void(value)> handler) {
		handlers.push( std::move(handler) );
		dataQueued.push_back(std::move(buff));

		if(dataWrited.empty()) {
			// start transmit process
			asyncWrite(asio::error_code(), 0);
		}
	}

	void base_client::asyncRead(const asio::error_code &ec, const size_t size) {
		if(ec || size == 0) {
			errorsubscribe_handler(ec.message());
			return;
		}

		for(size_t pos = 0; pos < size;) {
			std::pair<size_t, parser::ParseResult> result = redisParser.parse(buf.data() + pos, size - pos);

			if(result.second == parser::Completed) {
				doProcessMessage(std::move(redisParser.result()));
			} else if(result.second == parser::Incompleted) {
				processMessage();
				return;
			} else {
				errorsubscribe_handler("[RedisClient] Parser error");
				return;
			}

			pos += result.first;
		}

		processMessage();
	}

	void base_client::onRedisError(const value &v) {
		errorsubscribe_handler(v.toString());
	}

	void base_client::defaulErrorsubscribe_handler(const std::string &s) {
		throw std::runtime_error(s);
	}

	void base_client::append(std::vector<char> &vec, const buffer &buf) {
		vec.insert(vec.end(), buf.data(), buf.data() + buf.size());
	}

	void base_client::append(std::vector<char> &vec, const std::string &s) {
		vec.insert(vec.end(), s.begin(), s.end());
	}

	void base_client::append(std::vector<char> &vec, const char *s) {
		vec.insert(vec.end(), s, s + strlen(s));
	}

	void base_client::append(std::vector<char> &vec, char c) {
		vec.resize(vec.size() + 1);
		vec[vec.size() - 1] = c;
	}

	size_t base_client::subscribe(const std::string &command, const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		assert(state == State::Connected || state == State::Subscribed);

		if (state == State::Connected || state == State::Subscribed) {
			std::deque<buffer> items{command, channel};
			post(std::bind(&base_client::doAsyncCommand, this, makeCommand(items), std::move(handler)));
			msgsubscribe_handlers.insert(std::make_pair(channel, std::make_pair(subscribeSeq, std::move(msgsubscribe_handler))));
			state = State::Subscribed;

			return subscribeSeq++;
		} else {
			std::stringstream ss;
			ss << "base_client::subscribe called with invalid state " << to_string(state);

			errorsubscribe_handler(ss.str());
			return 0;
		}
	}

	void base_client::singleShotSubscribe(const std::string &command, const std::string &channel, std::function<void(std::vector<char> msg)> msgsubscribe_handler, std::function<void(value)> handler) {
		assert(state == State::Connected || state == State::Subscribed);

		if (state == State::Connected || state == State::Subscribed) {
			std::deque<buffer> items{command, channel};
			post(std::bind(&base_client::doAsyncCommand, this, makeCommand(items), std::move(handler)));
			singleShotMsgsubscribe_handlers.insert(std::make_pair(channel, std::move(msgsubscribe_handler)));
			state = State::Subscribed;
		} else {
			std::stringstream ss;
			ss << "base_client::singleShotSubscribe called with invalid state " << to_string(state);

			errorsubscribe_handler(ss.str());
		}
	}

	void base_client::unsubscribe(const std::string &command, size_t handleId, const std::string &channel, std::function<void(value)> handler) {
		#ifdef DEBUG
			static int recursion = 0;
			assert(recursion++ == 0);
		#endif

		assert(state == State::Connected || state == State::Subscribed);

		if (state == State::Connected || state == State::Subscribed) {
			// Remove subscribe-handler
			typedef base_client::Msgsubscribe_handlersMap::iterator iterator;
			std::pair<iterator, iterator> pair = msgsubscribe_handlers.equal_range(channel);

			for (iterator it = pair.first; it != pair.second;) {
			if (it->second.first == handleId) {
					msgsubscribe_handlers.erase(it++);
				} else {
					++it;
				}
			}

			std::deque<buffer> items{ command, channel };

			// Unsubscribe command for Redis
			post(std::bind(&base_client::doAsyncCommand, this, makeCommand(items), handler));
		} else {
			std::stringstream ss;

			ss << "base_client::unsubscribe called with invalid state " << to_string(state);

			#ifdef DEBUG
					--recursion;
			#endif
			errorsubscribe_handler(ss.str());
			return;
		}

		#ifdef DEBUG
			--recursion;
		#endif
	}
}

#endif
