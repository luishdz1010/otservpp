#ifndef OTSERVPP_SERVICEMANAGER_H_
#define OTSERVPP_SERVICEMANAGER_H_

#include <unordered_map>
#include "../networkdcl.hpp"
#include "../forwarddcl.hpp"

namespace otservpp {

/*! Entry point of network connections.
 * This tiny class is used for dispatching new connections to their respective services.
 */
class ServiceManager {
public:
	ServiceManager(boost::asio::io_service& ioService_);

	~ServiceManager() = default;

	/*! Starts all the services registered with this manager
	 * All the services will start listening in their respective port, but nothing will be
	 * done until the io_service given in the constructor is ran.
	 * \warning This function should only be called once on startup
	 */
	void start();

	/*! Adds the given service to the list of registered services
	 * There can only be one service with a particular port registered at the same time
	 * \warning Services can only be registered at startup time
	 */
	// TODO add support for late registering of services in multithreaded context
	void registerService(ServiceUniquePtr service);

	ServiceManager(ServiceManager&) = delete;
	void operator=(ServiceManager&) = delete;

private:
	/// This implements the network related data of a Service
	struct PrivateService{
		PrivateService(ServiceUniquePtr&& svc, boost::asio::io_service& ioService) :
			service(std::move(svc)),
			acceptor(ioService),
			peer(ioService)
		{}

		// FIXME find a way to make this move only
		//PrivateService(PrivateService&) = delete;
		PrivateService(PrivateService&&) = default;

		ServiceUniquePtr service;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket peer;
	};

	// TODO we don't need an u_map
	typedef std::unordered_map<uint16_t, PrivateService> PortServiceMap;

	/// Maintains the connection dispatching flow
	void acceptMore(PrivateService& svc);

	boost::asio::io_service& ioService;
	PortServiceMap serviceMap;
};

} /* namespace otservpp */
#endif // OTSERVPP_SERVICEMANAGER_H_
