#ifndef OTSERVPP_BASICPROTOCOL_H_
#define OTSERVPP_BASICPROTOCOL_H_

namespace otservpp {

/*! Base class for protocols
 * Provides default/dummy functions required by Connection, every function can be statically
 * overridden, but if you do override connectionMade(Connection<Protocol>*) you must call it in
 * the overriding function like \code BasicProtocol<MyProtocol>::connectionMade(conn) \endcode
 */
template <typename Protocol>
class BasicProtocol {
public:
	BasicProtocol() = default;
	BasicProtocol(BasicProtocol&&) = default;

	void connectionMade(Connection<Protocol>* conn)
	{
		peer = conn;
	}

	/// Should be implemented if the underlying protocol isn't a one-packet protocol
	void handleMessage()
	{
		assert(false);
	}

	BasicProtocol(BasicProtocol&) = delete;
	void operator=(BasicProtocol&) = delete;

protected:
	~BasicProtocol() = default;

	Connection<Protocol>* getConnection()
	{
		return peer;
	}

	template <class Message>
	void send(Message&& msg)
	{
		peer->send(std::forward<Message>(msg));
	}

private:
	Connection<Protocol>* peer;
};

} /* namespace netvent */

#endif // OTSERVPP_BASICPROTOCOL_H_
