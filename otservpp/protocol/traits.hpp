#ifndef OTSERVPP_PROTOCOLTRAITS_HPP_
#define OTSERVPP_PROTOCOLTRAITS_HPP_

#include "../forwarddcl.hpp"

namespace otservpp {

/// See connection.hpp for explanation
template <typename Protocol>
struct ProtocolTraits{
	typedef StandardInMessage IncomingMessage;
	typedef StandardOutMessage OutgoingMessage;

	enum{
		readTimeOut  = 30,
		writeTimeOut = 30
	};

	static const char* name(){ return "unnamed protocol"; }
};

} /* namespace otservpp */

#endif // OTSERVPP_PROTOCOLTRAITS_HPP_
