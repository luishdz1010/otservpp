#ifndef OTSERVPP_BASICPROTOCOL_HPP_
#define OTSERVPP_BASICPROTOCOL_HPP_

#include "connection.hpp"

namespace otservpp {

/*! Base class for all protocols
 * A protocol represents the intentions of a remote client by the means of a Connection
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

	void handlePacketException()
	{
		LOG(INFO) << "invalid packet received" << droppingLogInfo();
		connection->close();
	}

	std::string droppingLogInfo()
	{
		return connection->logInfo() + ", dropping connection";
	}

	ConnectionPtrType connection;
};

} /* namespace otservpp */

#endif // OTSERVPP_BASICPROTOCOL_HPP_
