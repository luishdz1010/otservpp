#ifndef OTSERVPP_BASICPROTOCOL_HPP_
#define OTSERVPP_BASICPROTOCOL_HPP_

#include "connection.hpp"
#include "../message/basicinmessage.hpp"

namespace otservpp {

/*! Base class for all protocols
 * A protocol represents the intentions of a remote client by the means of a Connection. This
 * class contains mostly helper constructions for use in protocol-related logic.
 */
template <class Protocol>
class BasicProtocol{
public:
	typedef ConnectionPtr<Protocol> ConnectionPtrType;

	BasicProtocol(const ConnectionPtrType& conn) :
		connection(conn)
	{}

	/// Overrides must call this base implementation,
	void connectionLost()
	{
		detachConnection();
	}

	/// This breaks the cyclic dependency on connection and protocol, preventing an otherwise
	/// nasty memory leak.
	void detachConnection()
	{
		connection.reset();
	}

	BasicProtocol(BasicProtocol&) = delete;
	void operator=(BasicProtocol&) = delete;

protected:
	~BasicProtocol() = default;

	/// Provides safe functor wrappers for use in asynchronous operations
	template <class Handler>
	struct WrappedHandler{
		Handler h;
		BasicProtocol* protocol;

		template <class... Args>
		void operator()(Args&&... args)
		{
			if(protocol->connection->isAlive()){
				try{
					h(std::forward<Args>(args)...);
				} catch(PacketException&){
					protocol->handlePacketException();
				} catch(std::exception& e){
					LOG(ERROR) << "unexpected exception caugh during normal proccessing"
							<< protocol->droppingLogInfo() << ". What: " << e.what();
					protocol->connection->stop();
				}
			} else {
				protocol->connection->stop();
			}
		}
	};

	/// In-place functor wrapping \sa WrappedHandler
	template <class Handler>
	WrappedHandler<Handler> wrapHandler(Handler&& handler)
	{
		return {std::forward<Handler>(handler), this};
	}

	/// Helper function for handling invalid packets
	void handlePacketException()
	{
		LOG(INFO) << "invalid packet received" << droppingLogInfo();
		connection->stop();
	}

	/// Used for logging proposes
	std::string droppingLogInfo()
	{
		return connection->droppingLogInfo();
	}

	ConnectionPtrType connection;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICPROTOCOL_HPP_
