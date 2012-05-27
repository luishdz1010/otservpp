#include "servicemanager.h"
#include <boost/lexical_cast.hpp>
#include "service.hpp"

using namespace boost::asio;
using namespace boost::asio::ip;

namespace otservpp {

DuplicatedServicePortException::DuplicatedServicePortException(const ServiceUniquePtr& s) :
	std::runtime_error("trying to register service " + s->getName() + " with a duplicated port "
			+ boost::lexical_cast<std::string>(s->getPort()))
{}

ServiceManager::ServiceManager(boost::asio::io_service& ioService_) :
	ioService(ioService_)
{}

void ServiceManager::start()
{
	// start listening on all acceptors
	for(auto& portNService : serviceMap){
		auto& acceptor = portNService.second.acceptor;
		tcp::endpoint endpoint {tcp::v4(), portNService.first};

		acceptor.open(endpoint.protocol());
		acceptor.set_option(socket_base::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();

		acceptMore(portNService.second);
	}
}

void ServiceManager::registerService(ServiceUniquePtr service_)
{
	PrivateService service {std::move(service_), ioService};

	if(serviceMap.emplace(service_->getPort(), std::move(service)).second)
		throw DuplicatedServicePortException(service_);
}

void ServiceManager::acceptMore(PrivateService& svc)
{
	svc.acceptor.async_accept(svc.peer, [this, &svc](const SystemError& e){
		if(!e){
			svc.service->incomingConnection(std::move(svc.peer));
			acceptMore(svc);
		} else{
			throw boost::system::system_error(e);
		}
	});
}

} /* namespace otservpp */