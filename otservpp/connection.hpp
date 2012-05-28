#ifndef OTSERVPP_CONNECTION_HPP_
#define OTSERVPP_CONNECTION_HPP_

#include <deque>
#include <glog/raw_logging.h>
#include "forwarddcl.hpp"
#include "networkdcl.hpp"
#include "protocol/traits.hpp"

namespace otservpp{

/*! Represents a connection with a remote peer.
 * The connection class takes care of managing the underlying message transmission. Everything
 * else is delegated to the Protocol class bounded to the connection.
 *
 * Protocol concept requirements:
 *  ProtocolTraitss<Protocol>::IncomingMessage Protocol::createIncomingMessage()
 *  	Will be called during the construction of the connection object, it should return an
 *  	empty message object that will be used for storing the incoming data.
 * 	void Protocol::connectionMade(Connection<Protocol>*)
 * 		Will be called right before ending the construction of the Connection object,
 * 	    The connection will outlive the protocol object
 * 	void Protocol::handleFirstMessage(ProtocolTraitss<P>::IncomingMessage[&])
 * 		Will be called when the first message arrives
 * 	void Protocol::handleMessage(ProtocolTraitss<P>::IncomingMessage[&])
 * 		Will be called when any subsequent message arrives
 * 	std::string Protocol::getName()
 * 		Should return a human readable id (without the " protocol" suffix)
 *
 * A specialization of trait template ProtocolTraits can be provided for customization.
 *  ProtocolTraitss<P>::IncomingMessage
 * 		A message/buffer type used to store incoming bytes from the remote peer.
 * 	ProtocolTraitss<P>::OutgoingMessage
 * 		A message/buffer type used to store outgoing bytes to a remote peer
 * 	int ProtocolTraitss<P>::readTimeout
 * 		Should be set to the time in seconds this connection will wait on read operations before
 * 		timing out
 * 	int ProtocolTraitss<P>::writeTimeout
 * 		Should be set to the time in seconds this connection will wait on write operations before
 * 		timing out
 */
template <class Protocol>
class Connection : public std::enable_shared_from_this<Connection<Protocol>> {
public:
	using std::enable_shared_from_this<Connection>::shared_from_this;

	typedef typename ProtocolTraits<Protocol>::IncomingMessage IncomingMessage;
	typedef typename ProtocolTraits<Protocol>::OutgoingMessage OutgoingMessage;

	enum{
		READ_TIMEOUT  = ProtocolTraits<Protocol>::readTimeOut,
		WRITE_TIMEOUT = ProtocolTraits<Protocol>::writeTimeOut
	};

	Connection(TcpSocket&& socket, Protocol&& protocol_) :
		peer(std::move(socket)),
		protocol(std::move(protocol_))
	{
		LOG(INFO) << "connection made from " << peer.remote_endpoint();
		// pass the connection to the protocol for storing,
		// some protocols like game send a packet right after connecting
		protocol.connectionMade(*this);
	}

	/// The connection is destroyed whenever an error occurs or a call to close() is made
	~Connection()
	{
		assert(isStopped());
	}

	/// Starts listening for incoming data
	void start()
	{
		parseIncomingMessage<ProtocolFirstMessageHandler>();
	}

	/// Closes the connection and stops any pending IO operation
	void stop()
	{
		assert(!isStopped());
		peer.shutdown(TcpSocket::shutdown_both);
		peer.close();
	}

	bool isStopped()
	{
		return !peer.is_open();
	}

	/*! Sends a message to the remote peer asynchronously.
	 * The given message will be passed to this Protocol::OutgoingMessage constructor. An
	 * outgoing message queue is maintained by this class in order to process messages
	 * sequentially.
	 * \note This functions is thread-safe
	 */
	template <class Message>
	void send(Message&& msg)
	{
		strand.dispatch([this, msg]{
			if(!isStopped()) return;

			bool shallSend = outMsgQueue.empty();

			outMsgQueue.emplace_back(std::forward<Message>(msg));

			if(shallSend)
				doSend();
		});
	}

	Connection(Connection&) = delete;
	void operator=(Connection&) = delete;

private:
	/// Helper for dispatching the first messages to the protocol
	struct ProtocolFirstMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol.handleFirstMessage(c->inMsg); };
	};

	/// Helper for dispatching subsequent messages to the protocol
	struct ProtocolMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol.handleMessage(c->inMsg); };
	};


	/*! Constructs a incoming message and call its respective protocol handler.
	 * On success calls Protocol::handleFirstMessage or Protocol Protocol::handleMessage, depending
	 * on whether this is the first or a subsequent message coming from the remote peer.
	 */
	template <class ProtocolHandler>
	void parseIncomingMessage()
	{
		auto t = shared_from_this();

		updateReadTimer();

		// strand wrapping in asyncRead. note: this insider lambdas is needed for some reason in gcc
		asyncRead(inMsg.getHeaderBuffer(), [=]{
			this->updateReadTimer();

			this->asyncRead(inMsg.parseHeaderAndGetBodyBuffer(), [this, t]{
				inMsg.parseBody();

				try{
					ProtocolHandler::sendInMsg(this);
				}catch(std::exception& e){
					LOG(ERROR) << "exception caught during " << protocol.getName()
							<< " protocol execution, dropping connection. what: " << e.what();
					return stop();
				}

				// stop maybe called from inside the handler
				if(!isStopped())
					this->parseIncomingMessage<ProtocolMessageHandler>();
			});
		});
	}

	/// Keeps the read timer alive
	void updateReadTimer()
	{
		readTimer.expires_from_now(boost::posix_time::seconds(READ_TIMEOUT));
		readTimer.async_wait(strand.wrap([this](const SystemError &e){ this->abort(e); }));
	}

	/// Helper function for async socket reading
	template <class Buffer, class Lambda>
	void asyncRead(Buffer&& buffer, Lambda&& body)
	{
		boost::asio::async_read(peer, std::forward<Buffer>(buffer),
		strand.wrap([this, body](const SystemError& e, std::size_t bytesRead){
			if(!e)
				body();
			else
				this->abort(e);
		}));
	}

	/// Called whenever an IO error or timeout occurs
	void abort(const SystemError& e = SystemError())
	{
		// timer.cancel() ends with operation_aborted, so we can reuse this function for timers
		if(e != boost::asio::error::operation_aborted)
			stop();
	}

	/// Performs the actual send operation, one message at a time
	void doSend()
	{
		writeTimer.expires_from_now(boost::posix_time::seconds(WRITE_TIMEOUT));
		writeTimer.async_wait(strand.wrap([this](const SystemError& e){ this->abort(e); }));

		boost::asio::async_write(peer, boost::asio::buffer(outMsgQueue.front().getBuffer()),
		strand.wrap([this](const SystemError& e) mutable{ // random lambda bug in gcc 4.7 fix
			if(!e){
				writeTimer.cancel();

				outMsgQueue.pop_front();

				if(!outMsgQueue.empty())
					doSend();
			} else {
				this->abort(e);
			}
		}));
	}


	TcpSocket peer;
	Protocol protocol;
	boost::asio::strand strand {peer.get_io_service()};
	boost::asio::deadline_timer readTimer {peer.get_io_service()};
	boost::asio::deadline_timer writeTimer {peer.get_io_service()};
	IncomingMessage inMsg {protocol.createIncomingMessage()};
	std::deque<OutgoingMessage> outMsgQueue;
};

} /* namespace otservpp */

#endif // OTSERVPP_CONNECTION_HPP_
