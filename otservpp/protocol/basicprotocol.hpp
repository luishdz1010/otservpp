#ifndef OTSERVPP_BASICPROTOCOL_H_
#define OTSERVPP_BASICPROTOCOL_H_

#include "../connection.hpp"

namespace otservpp {

/*! Base class for protocols
 * Provides default/dummy functions required by Connection, every function can be statically
 * overridden.
 */
template <typename Protocol>
class BasicProtocol : public Connection<Protocol>{
public:
	BasicProtocol(boost::asio::ip::tcp::socket&& socket) :
		Connection<Protocol>(socket.get_io_service(), std::move(socket))
	{}

	/// Should be implemented if the underlying protocol isn't a one-packet protocol
	void handleMessage()
	{
		assert(false);
	}

	BasicProtocol(BasicProtocol&) = delete;
	void operator=(BasicProtocol&) = delete;

protected:
	~BasicProtocol() = default;
};

} /* namespace netvent */

#endif // OTSERVPP_BASICPROTOCOL_H_
