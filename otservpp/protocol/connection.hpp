#ifndef OTSERVPP_CONNECTION_HPP_
#define OTSERVPP_CONNECTION_HPP_

#include <deque>
#include <glog/logging.h>
#include "forwarddcl.hpp"
#include "networkdcl.hpp"
#include "protocol/traits.hpp"

namespace otservpp{

/*! Represents a connection with a remote peer.
 * The connection class takes care of managing the underlying message transmission. This class
 * is meant to be CRTP'd, the subclasses (i.e. protocols) must implement:
 *
 * 	void Protocol::handleFirstMessage(ProtocolTraits<Protocol>::IncomingMessage[&])
 * 		Called when the first message arrives
 * 	void Protocol::handleMessage(ProtocolTraits<Protocol>::IncomingMessage[&])
 * 		Called when any subsequent message arrives
 * 	std::string Protocol::getName()
 * 		Should return a short human readable protocol description e.g. "login protocol"
 *
 * A specialization of trait template ProtocolTraits can be provided for further customization.
 *  ProtocolTraitss<P>::IncomingMessage
 * 		A message/buffer type used to store incoming bytes from a remote peer
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
class Connection : public std::enable_shared_from_this<Protocol> {
public:
	typedef typename ProtocolTraits<Protocol>::IncomingMessage IncomingMessage;
	typedef typename ProtocolTraits<Protocol>::OutgoingMessage OutgoingMessage;

	enum{
		READ_TIMEOUT  = ProtocolTraits<Protocol>::readTimeOut,
		WRITE_TIMEOUT = ProtocolTraits<Protocol>::writeTimeOut
	};

	Connection(boost::asio::io_service& ioService, boost::asio::ip::tcp::socket&& socket) :
		strand(ioService),
		peer(std::move(socket)),
		readTimer(ioService),
		writeTimer(ioService)
	{
		VLOG(1) << "connection made " << logInfo();
	}

	/// Starts listening for incoming data
	void start()
	{
		parseIncomingMessage<ProtocolFirstMessageHandler>();
	}

	bool isStopped()
	{
		return !peer.is_open();
	}

	boost::asio::ip::address getPeerAddress()
	{
		return peer.remote_endpoint().address();
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
		if(isStopped())
			return;

		strand.dispatch([=]{
			if(!isStopped()) return;

			DVLOG(1) << "queuing message " << outMsgQueue.front() << logInfo();

			bool shallSend = outMsgQueue.empty();

			outMsgQueue.emplace_back(std::forward<Message>(msg));

			if(shallSend)
				doSend();
		});
	}

	Connection(Connection&) = delete;
	void operator=(Connection&) = delete;

protected:
	using std::enable_shared_from_this<Protocol>::shared_from_this;

	/// The connection is destroyed whenever an error occurs or a call to close() is made
	~Connection()
	{
		assert(isStopped());
	}

	/// Closes the connection and stops any pending IO operation, subclasses that "override" this
	/// must call this base implementation
	void stop()
	{
		VLOG(1) << "stopping connection" << logInfo();

		assert(!isStopped());
		peer.shutdown(TcpSocket::shutdown_both);
		peer.close();
		readTimer.cancel();
		writeTimer.cancel();
	}

	std::ostrstream logInfo()
	{
		return std::ostrstream()
			<< " on " << getRemoteEndpoint() << " from " << protocol()->getName();
	}

private:
	/// Helper for dispatching the first messages to the protocol
	struct ProtocolFirstMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol()->handleFirstMessage(inMsg); };
	};

	/// Helper for dispatching subsequent messages to the protocol
	struct ProtocolMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol()->handleMessage(inMsg); };
	};

	Protocol* protocol()
	{
		return static_cast<Protocol*>(this);
	}

	/*! Constructs a incoming message and call its respective protocol handler.
	 * On success calls Protocol::handleFirstMessage or Protocol::handleMessage, depending
	 * on whether this is the first or a subsequent message coming from the remote peer.
	 */
	template <class ProtocolHandler>
	void parseIncomingMessage()
	{
		updateReadTimer();

		auto sthis = shared_from_this();

		// strand wrapping in asyncRead. note: this insider lambdas is needed for some reason in gcc
		asyncRead(inMsg.getHeaderBuffer(), [=]{
			this->updateReadTimer();

			this->asyncRead(inMsg.parseHeaderAndGetBodyBuffer(), [=]{
				assert(sthis);
				this->readTimer.cancel();
				inMsg.parseBody();

				DVLOG(1) << "handling message " << inMsg << logInfo();

				try{
					ProtocolHandler::sendInMsg(this);
				}catch(std::exception& e){
					LOG(ERROR) << "exception caught during " << protocol.getName()
							<< " execution, dropping connection. What: " << e.what();
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
		readTimer.async_wait(strand.wrap([this](const SystemErrorCode &e){
			DLOG(ERROR) << "read operation timedout after " << READ_TIMEOUT << "s" << logInfo();
			this->abort(e);
		}));
	}

	/// Keeps the write timer alive
	void updateWriteTimer()
	{
		writeTimer.expires_from_now(boost::posix_time::seconds(WRITE_TIMEOUT));
		writeTimer.async_wait(strand.wrap([this](const SystemErrorCode &e){
			DLOG(ERROR) << "write operation timedout after " << WRITE_TIMEOUT << "s" << logInfo();
			this->abort(e);
		}));
	}

	/// Helper function for async socket reading
	template <class Buffer, class Lambda>
	void asyncRead(Buffer&& buffer, Lambda&& body)
	{
		boost::asio::async_read(peer, std::forward<Buffer>(buffer),
		strand.wrap([this, body](const SystemErrorCode& e, std::size_t bytesRead){
			if(!e){
				DVLOG(1) << bytesRead << " bytes read" << logInfo();
				body();
			} else {
				DLOG(ERROR) << "read error " << e << " after " << bytesRead << " bytes read"
						<<  logInfo();
				this->abort(e);
			}
		}));
	}

	/// Called whenever an IO error or timeout occurs
	void abort(const SystemErrorCode& e = SystemErrorCode())
	{
		// timer.cancel() ends with operation_aborted, so we can reuse this function for timers
		if(e != boost::asio::error::operation_aborted)
			stop();
	}

	/// Performs the actual send operation, one message at a time
	void doSend()
	{
		updateWriteTimer();

		auto sthis = shared_from_this();

		boost::asio::async_write(peer, outMsgQueue.front().getBuffer(),
		strand.wrap([=](const SystemErrorCode& e, std::size_t bytesWritten) mutable {
			if(!e){
				writeTimer.cancel();

				DVLOG(1) << bytesWritten << " bytes written" << logInfo();

				outMsgQueue.pop_front();

				if(!outMsgQueue.empty())
					doSend();
				else
					writeTimer.cancel();

			} else {
				DLOG(ERROR) << "write error " << e << " after " << bytesWritten << " bytes written"
						<< logInfo();
				this->abort(e);
			}
		}));
	}

	IncomingMessage inMsg;
	std::deque<OutgoingMessage> outMsgQueue;
	boost::asio::ip::tcp::socket peer;
	boost::asio::strand strand;
	boost::asio::deadline_timer readTimer;
	boost::asio::deadline_timer writeTimer;
};

} /* namespace otservpp */

#endif // OTSERVPP_CONNECTION_HPP_
