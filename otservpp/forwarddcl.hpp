#ifndef OTSERVPP_FORWARDDCL_HPP_
#define OTSERVPP_FORWARDDCL_HPP_

#include <memory>

// some dirty hacks
namespace std{

template <class T>
T* get_pointer(const std::shared_ptr<T>& ptr)
{
	return ptr.get();
}

template <class T>
T* get_pointer(const std::unique_ptr<T>& ptr)
{
	return ptr.get();
}

template <class T, class... Args>
std::unique_ptr<T> make_uniqe(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} /* namespace std */

namespace otservpp{

template <class Protocol>
class Connection;

template <class Protocol>
using ConnectionPtr = std::shared_ptr<Connection<Protocol>>;

class Service;
class ServiceManager;

typedef std::unique_ptr<Service> ServiceUniquePtr;

class StandardInMessage;
class StandardOutMessage;

class DeferredTask;
class IntervalTask;
class Dispatcher;

typedef std::shared_ptr<DeferredTask> DeferredTaskPtr;
typedef std::shared_ptr<IntervalTask> IntervalTaskPtr;

class LoginProtocol;

template <class Protocol>
using ProtocolPtr = std::shared_ptr<Protocol>

} /* namespace otservpp */

#endif // OTSERVPP_FORWARDDCL_HPP_
