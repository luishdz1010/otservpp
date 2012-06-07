#ifndef OTSERVPP_CONNECTION_HPP_
#define OTSERVPP_CONNECTION_HPP_

#include <deque>
#include <glog/logging.h>
#include "../forwarddcl.hpp"
#include "../networkdcl.hpp"
#include "traits.hpp"
#include <bitset>

namespace otservpp{

/*! Represents a connection with a remote peer.
 * The connection class takes care of managing the underlying message transmission from and to
 * a remote peer. This is independent from the message interpreter (i.e. protocol) used.
 *
 * The underlying protocol can be modified at any time during the program execution, this however
 * requires having an abstract Protocol base class. While this isn't common operation, it allow
 * us to code a far more natural design regarding the use of protocols and connections. For
 * instance, we can separate the login-queue code in LoginQueueProtocol from the actual in-game
 * character manipulation in GameProtocol, we can also
 *
 * The ConnectionT template parameter is used as argument in ConnectionTraits to configure the
 * types and behavior of the Connection. See ConnectionTraits for details.
 *
 * This class is designed to be thread safe (i.e multiple threads can send messages
 * through the same instance), but only the connection's thread will be receiving messages.
 * Even after calling Connection::stop() the connection might still call protocol functions.
 * \see Protocol ConnectionTraits
 */
template <class Protocol>
class Connection : public std::enable_shared_from_this<Connection<Protocol> > {

	// help us to switch protocols at runtime
	struct ConnectionImpl{
		ConnectionImpl(boost::asio::io_service& ioSvc, boost::asio::ip::tcp::socket&& socket) :
			peer(std::move(socket)),
			strand(ioSvc),
			readTimer(ioSvc),
			writeTimer(ioSvc)
		{}

		boost::asio::ip::tcp::socket peer;
		boost::asio::strand strand;
		boost::asio::deadline_timer readTimer;
		boost::asio::deadline_timer writeTimer;
	};

	// connection state
	enum{
		ReadClosed = 0x01,
		WriteClosed = 0x02,
		//ProtocolMoved = 0x4
	};

public:
	typedef std::shared_ptr<Protocol> ProtocolPtr;
	typedef ProtocolTraits<Protocol> TypeTraits;
	typedef typename TypeTraits::IncomingMessage IncomingMessage;
	typedef typename TypeTraits::OutgoingMessage OutgoingMessage;

	enum{
		ReadTimeOut  = TypeTraits::ReadTimeOut,
		WriteTimeOut = TypeTraits::WriteTimeOut
	};

	/*! Creates a connection with the given socket representing the remote peer
	 * \note Asio sockets can't be ioservice re-parented, this could cause problems if we want
	 * to change the connection multiplexing thread, at the moment we don't do that (and it's
	 * probably we'll never do that anyway)
	 */
	Connection(boost::asio::ip::tcp::socket&& socket) :
		impl(new ConnectionImpl(socket.get_io_service(), std::move(socket)))
	{
		VLOG(1) << "connection made" << logInfo();
	}

	/*! Creates a new Connection with a different Protocol "stealing" another connection's peer
	 * This can be used to switch protocols at runtime, however some guarantees have to be meet:
	 *  \li The given connection's functions stop(), stopReceiving() and stopSending() weren't
	 *  	called already
	 *	\li The given connection can't be already sending anything
	 *	\li This function is called from a function that is on the call stack of
	 *		OldProtocol::handleFirstMessage or OldProtocol::handleMessage
	 * After returning from this function the old connection is unusable. To start receiving
	 * data from the remote peer Connection::start() must be called on the new connection,
	 * as normally Protocol::handleFirstMessage() will be called when receiving the first
	 * message
	 */
	template <class OldProtocol>
	Connection(Connection<OldProtocol>&& old) :
		impl(old.impl.release())
	{
		assert(old.outMsgQueue.empty());
		assert(!old.isStopped());

		DLOG(INFO) << "switching protocol from" << old.protocol->getName() << logInfo();

		// after returning from OldProtocol::handle[First]Message 'isStopped()' must return
		// true, we can't use old.stop() since it closes the socket
		old.closeStatus = ReadClosed | WriteClosed;

		impl->readTimer.cancel(); // canceling doesn't change deadline
		waitOnReadTimer(); // same handler different 'this'

		impl->writeTimer.cancel();
		waitOnWriteTimer();

		start();
	}

	/// The connection is destroyed whenever an error occurs or a call to close() is made
	~Connection()
	{
		assert(isStopped());
	}

	/// Starts listening for incoming data passing all completed messages to the given protocol
	void start(const ProtocolPtr& p)
	{
		protocol = p;
		DVLOG(1) << "starting reading" << logInfo();
		parseIncomingMessage<ProtocolFirstMessageHandler>();
	}

	/// Closes the connection, stops any pending IO operation and resets the protocol ptr
	/// \note This function is thread-safe
	void stop()
	{
		closeStatus = ReadClosed | WriteClosed;
		impl->strand.dispatch([this]{
			VLOG(1) << "stopping connection" << this->logInfo();
			impl->readTimer.cancel();
			impl->writeTimer.cancel();
			impl->peer.shutdown(TcpSocket::shutdown_both);
			impl->peer.close();
			protocol.reset();
		});
	}

	/// TODO decide if atomic closeStatus is worth it
	void stopReceiving()
	{
		closeStatus |= ReadClosed;
		impl->peer.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
	}

	void stopSending()
	{
		closeStatus |= WriteClosed;
		impl->peer.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
	}

	bool isReceiving()
	{
		return ~closeStatus & ReadClosed;
	}

	bool isSendind()
	{
		return ~closeStatus & WriteClosed;
	}

	/*! Checks whether a connection is stopped
	 * A connection is stopped when it ain't receiving nor sending data.
	 */
	bool isStopped()
	{
		return closeStatus & (ReadClosed | WriteClosed);
	}

	/// Returns the peers IP address
	boost::asio::ip::address getPeerAddress()
	{
		return impl->peer.remote_endpoint().address();
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
		trySendOrEnqueueThen(std::forward<Message>(msg), [this]{ keepSending(); });
	}

	/// Same as send(Message&& msg) but stops the connection when finished
	/// \note This functions is thread-safe
	template <class Message>
	void sendAndStop(Message&& msg)
	{
		trySendOrEnqueueThen(std::forward<Message>(msg), [this]{ stop(); });
	}

	Connection(Connection&) = delete;
	void operator=(Connection&) = delete;

private:
	using std::enable_shared_from_this<Connection>::shared_from_this;

	const char* logInfo()
	{
		std::ostrstream s;
		s << " on " << getPeerAddress() << " with "
				<< (protocol? protocol->getName() : "no protocol");
		 return s.str();
	}

	/// Helper for dispatching the first messages to the protocol
	struct ProtocolFirstMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol->handleFirstMessage(c->inMsg); };
	};

	/// Helper for dispatching subsequent messages to the protocol
	struct ProtocolMessageHandler{
		static void sendInMsg(Connection* c){ c->protocol->handleMessage(c->inMsg); };
	};

	/*! Constructs a incoming message and call its respective protocol handler.
	 * On success calls Protocol::handleFirstMessage or Protocol::handleMessage, depending
	 * on whether this is the first or a subsequent message coming from the remote peer.
	 */
	template <class ProtocolHandler>
	void parseIncomingMessage()
	{
		updateReadTimer();

		auto sthis = shared_from_this();

		// strand wrapping is in asyncRead.
		// note: 'this->func(x)' inside lambdas is needed for some reason in gcc
		asyncRead(inMsg.getHeaderBuffer(), [=](){
			this->updateReadTimer();

			this->asyncRead(inMsg.parseHeaderAndGetBodyBuffer(), [=](){
				assert(sthis);
				impl->readTimer.cancel();
				inMsg.parseBody();

				//DVLOG(1) << "handling message " << inMsg << this->logInfo();

				try{
					ProtocolHandler::sendInMsg(this);
				}catch(std::exception& e){
					LOG(ERROR) << "exception caught during "
							<< (protocol? protocol->getName() : "no protocol")
							<< " execution, dropping connection. What: " << e.what();
					return this->stop();
				}

				if(this->isReceiving())
					this->parseIncomingMessage<ProtocolMessageHandler>();
			});
		});
	}

	/// Helper function for async socket reading
	template <class Buffer, class Lambda>
	void asyncRead(Buffer&& buffer, Lambda&& body)
	{
		boost::asio::async_read(impl->peer, std::forward<Buffer>(buffer),
		impl->strand.wrap([this, body](const SystemErrorCode& e, std::size_t bytesRead){
			if(!e){
				if(!this->isReceiving()) return;

				DVLOG(1) << bytesRead << " bytes read" << this->logInfo();

				body();
			} else {
				DLOG_IF(ERROR, e != boost::asio::error::operation_aborted) << "read error "
						<< e << " after " << bytesRead << " bytes read" <<  this->logInfo();
				this->abort(e);
			}
		}));
	}

	/// Keeps the read timer alive
	void updateReadTimer()
	{
		impl->readTimer.expires_from_now(boost::posix_time::seconds(ReadTimeOut));
		waitOnReadTimer();
	}

	void waitOnReadTimer()
	{
		impl->readTimer.async_wait(impl->strand.wrap([this](const SystemErrorCode &e){
			DLOG_IF(ERROR, e != boost::asio::error::operation_aborted)
				<< "read operation timedout after " << ReadTimeOut << "s" << this->logInfo();
			this->abort(e);
		}));
	}

	/// Keeps the write timer alive
	void updateWriteTimer()
	{
		impl->writeTimer.expires_from_now(boost::posix_time::seconds(WriteTimeOut));
		waitOnWriteTimer();
	}

	void waitOnWriteTimer()
	{
		impl->writeTimer.async_wait(impl->strand.wrap([this](const SystemErrorCode &e){
			DLOG_IF(ERROR, e != boost::asio::error::operation_aborted)
				<< "write operation timedout after " << WriteTimeOut << "s" << this->logInfo();
			this->abort(e);
		}));
	}

	/// Called whenever an IO error or timeout occurs
	void abort(const SystemErrorCode& e = SystemErrorCode())
	{
		// timer.cancel() ends with operation_aborted, so we can reuse this function for timers
		if(e != boost::asio::error::operation_aborted){
			protocol->connectionLost();
			stop();
		}
	}

	template <class Message, class AfterSend>
	void trySendOrEnqueueThen(Message&& msg, AfterSend&& then)
	{
		impl->strand.dispatch([=]{
			if(!isSendind()) return;

			DVLOG(1) << "queuing message " << outMsgQueue.front() << logInfo();

			bool shallSend = outMsgQueue.empty();

			outMsgQueue.emplace_back(std::forward<Message>(msg));

			if(shallSend)
				doSend(then);
		});
	}

	/// Performs the actual send operation, one message at a time
	template <class AfterSend>
	void doSend(AfterSend&& then)
	{
		updateWriteTimer();

		auto sthis = shared_from_this();

		boost::asio::async_write(impl->peer, outMsgQueue.front().getBuffer(),
		impl->strand.wrap([=](const SystemErrorCode& e, std::size_t bytesWritten) mutable {
			if(!e){
				DVLOG(1) << bytesWritten << " bytes written" << logInfo();

				outMsgQueue.pop_front();

				then(); // keepSending() or close(), see send/sendAndClose

			} else {
				DLOG_IF(ERROR, e != boost::asio::error::operation_aborted) << "write error "
					<< e << " after " << bytesWritten << " bytes written" << logInfo();
				this->abort(e);
			}
		}));
	}

	void keepSending()
	{
		if(!outMsgQueue.empty())
			doSend([this]{ keepSending(); });
		else
			impl->writeTimer.cancel();
	}

	// protocol dependent types
	IncomingMessage inMsg;
	std::deque<OutgoingMessage> outMsgQueue;
	ProtocolPtr protocol;
	//std::atomic_int closeStatus {0};

	std::unique_ptr<ConnectionImpl> impl;

	char closeStatus {0};
};

} /* namespace otservpp */

#endif // OTSERVPP_CONNECTION_HPP_
