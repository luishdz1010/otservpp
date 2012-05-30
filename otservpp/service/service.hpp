#ifndef OTSERVPP_SERVICE_HPP_
#define OTSERVPP_SERVICE_HPP_

#include "../forwarddcl.hpp"
#include "../networkdcl.hpp"

namespace otservpp {

/*! Represents a listener on a network port.
 * The ServiceManager class is responsible for the handling of new connections, services only
 * listen for them.
 * A protocol is attached to a service, either by subclassing BasicService or Service.
 */
class Service{
public:
	Service() = default;
	virtual ~Service() = default;

	/// Human readable name of the service, used for logging proposes
	virtual const std::string& getName() const = 0;

	/// The port this service wants to listen to
	virtual uint16_t getPort() const = 0;

	/// Called when a new connection arrives in Service::getPort()
	/// \note Implementation must be thread safe
	virtual void incomingConnection(boost::asio::ip::tcp::socket&&) = 0;
};

/// Common protocol base class
template <class Protocol>
class BasicService : public Service{
public:
	typedef ConnectionPtr<Protocol> ConnectionType;

	BasicService(uint16_t port_) :
		port(port_)
	{}

	~BasicService() = default;

	uint16_t getPort() const final override
	{
		return port;
	}

	const std::string& getName() const final override
	{
		static std::string name {Protocol::name(), " ", " service"};
		return name;
	}

	void incomingConnection(boost::asio::ip::tcp::socket&& socket) final override
	{
		auto connection = std::make_shared<Connection<Protocol>>(std::move(socket));
		auto protocol = makeProtocol(connection);
		connection->start(protocol);
	}

protected:
	/// Override for custom protocol creation
	virtual ProtocolPtr<Protocol> makeProtocol(const ConnectionType& connection)
	{
		return std::make_shared<Protocol>(connection);
	}

	uint16_t port;
};

} /* namespace otservpp */

#endif // OTSERVPP_SERVICE_HPP_
