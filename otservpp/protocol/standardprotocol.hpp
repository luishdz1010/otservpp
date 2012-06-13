#ifndef OTSERVPP_STANDARDPROTOCOL_HPP_
#define OTSERVPP_STANDARDPROTOCOL_HPP_

#include "basicprotocol.hpp"
#include "../message/standardinmessage.h"
#include "../message/standaroutmessage.h"

namespace otservpp{

/*! Standard tibia protocol base
 * This class implements some of the common procedures in the standard Tibia protocols, like
 * XTEA encryption.
 */
template <class Protocol>
class StandardProtocol : public BasicProtocol<Protocol>{
public:
	typedef typename BasicProtocol<Protocol>::ConnectionPtrType ConnectionPtrType;

	StandardProtocol(const ConnectionPtrType& conn) :
		BasicProtocol(conn)
	{}

	void handleFirstMessage(StandardInMessage& msg)
	{
		try{
			msg.doChecksum();
			static_cast<Protocol*>(this)->onFirstMessage(msg);
		} catch(PacketException& e){
			handlePacketException();
		}
	}

	void handleMessage(StandardInMessage& msg)
	{
		try{
			msg.doChecksum();
			msg.xteaDecrypt(xtea);
			static_cast<Protocol*>(this)->onMessage(msg);
		} catch(PacketException& e){
			handlePacketException();
		}
	}

protected:
	~StandardProtocol() = default;

	template <class Handler>
	struct WrappedHandler{
		Handler h;
		StandardProtocol* protocol;

		template <class... Args>
		void operator()(Args&&... args)
		{
			if(protocol->connection->isAlive()){
				try{
					h(std::forward<Handler>(args));
				} catch(PacketException&){
					handlePacketException();
				} catch(std::exception& e){
					LOG(ERROR) << "exception caugh during normal proccessing"
							<< droppingLogInfo() << ". What: " << e.what();
				}
			} else {
				connection->stop();
			}
		}
	};

	template <class Handler>
	WrappedHandler<Handler> wrapHandler(Handler&& handler)
	{
		return {std::forward<Handler>(handler), this};
	}

	/// Placeholder implementation for one-message protocols, should never be called though
	void onMessage(StandardInMessage& msg)
	{
		assert(false);
	}

	void setXtea(const crypto::Xtea& xtea_)
	{
		xtea = xtea_;
	}

	crypto::Xtea& getXtea()
	{
		return xtea;
	}

	void sendUnencrypted(StandardOutMessage& msg)
	{
		msg.addHeader();
		connection->send(msg);
	}

	void send(StandardOutMessage& msg)
	{
		msg.addHeader();
		msg.xteaEncrypt(xtea);
		connection->send(msg);
	}

	void sendAndStop(StandardOutMessage& msg)
	{
		msg.addHeader();
		msg.xteaEncrypt(xtea);
		connection->sendAndClose(msg);
	}

private:
	crypto::Xtea xtea;
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDPROTOCOL_HPP_
