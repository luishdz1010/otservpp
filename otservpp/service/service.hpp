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
	virtual void incomingConnection(boost::asio::ip::tcp::socket&&) = 0;
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
		static std::string name {Protocol::name(), " ", " service"};
		return name;
	}

	void incomingConnection(boost::asio::ip::tcp::socket&& socket) final override
	{
		makeProtocol()->start();
	}

private:
	/// Override for custom protocol creation
	virtual std::shared_ptr<Protocol> makeProtocol(boost::asio::ip::tcp::socket&& socket)
	{
		return std::make_shared<Protocol>(socket.get_io_service(), std::move(socket));
	}

	uint16_t port;
};

} /* namespace otservpp */

#endif // OTSERVPP_SERVICE_HPP_
