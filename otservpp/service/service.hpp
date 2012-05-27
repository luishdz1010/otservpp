#ifndef OTSERVPP_SERVICE_HPP_
#define OTSERVPP_SERVICE_HPP_

#include "../forwarddcl.hpp"
#include "../networkdcl.hpp"
#include "../protocol/traits.hpp"

namespace otservpp {

/*! Represents a listener on a network port.
 * The ServiceManager class is responsible for the handling of new connections, services only
 * listen for them.
 * Every protocol is attached to a service, either by subclassing BasicService or Service.
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
	virtual void incomingConnection(TcpSocket) = 0;
};

template <class Protocol>
class BasicService : public Service{
public:
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
		static std::string name {ProtocolTraits<Protocol>::name(), " ", " service"};
		return name;
	}

	void incomingConnection(TcpSocket socket) const final override
	{
		auto conn = std::make_shared<Connection<Protocol>>(std::move(socket), makeProtocol());
		conn.start(); // the connection lives by registering itself with the socket's io_service
	}

protected:
	/// Override this in subclasses for custom protocol creation
	/// \note Implementation must be thread safe
	virtual Protocol makeProtocol()
	{
		return Protocol();
	}

private:
	uint16_t port;
};

} /* namespace otservpp */

#endif // OTSERVPP_SERVICE_HPP_
