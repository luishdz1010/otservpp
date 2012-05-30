#ifndef OTSERVPP_BASICPROTOCOL_HPP_
#define OTSERVPP_BASICPROTOCOL_HPP_

#include "connection.hpp"

namespace otservpp {

/*! Base class for most protocols
 * A protocol represents the intentions of a remote peer by means of a Connection
 */
template <typename Protocol>
class BasicProtocol{
public:
	typedef ConnectionPtr<Protocol> ConnectionPtrType;

	BasicProtocol(const ConnectionPtrType& conn) :
		connection(conn)
	{}

	/// Should be implemented if the underlying protocol isn't a one-packet protocol
	void handleMessage(const typename ProtocolTraits<Protocol>::IncomingMessage&)
	{
		assert(false);
	}

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

	ConnectionPtrType connection;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICPROTOCOL_HPP_
