#ifndef OTSERVPP_STANDARDPROTOCOL_HPP_
#define OTSERVPP_STANDARDPROTOCOL_HPP_

#include "basicprotocol.hpp"
#include "../message/standardinmessage.h"

namespace otservpp{

/*! Standard tibia protocol base
 * This class implements some of the common procedures in the standard tibia protocols, like
 * encrypting and checksum check.
 */
template <class Protocol>
class StandardProtocol : public BasicProtocol<Protocol>{
public:
	typedef typename BasicProtocol<Protocol>::ConnectionPtrType ConnectionPtrTpe;

	StandardProtocol(const ConnectionPtrTpe& conn) :
		BasicProtocol<Protocol>(conn)
	{}

	void setXtea(const crypto::Xtea& xtea_)
	{
		xtea = xtea_;
	}

	crypto::Xtea& getXtea()
	{
		return xtea;
	}

	// TODO move this to StandardOutMessage
	/*void encryptXTEA(StandardOutMessage& msg)
	{
		msg.addPadding(8 - msg.getSize()%8);

		uint32_t* buffer = msg.getBuffer<uint32_t>();
	}*/

protected:
	~StandardProtocol() = default;

private:
	crypto::Xtea xtea;
};

} /* namespace otservpp */

#endif // OTSERVPP_STANDARDPROTOCOL_HPP_
