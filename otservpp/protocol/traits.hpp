#ifndef OTSERVPP_PROTOCOLTRAITS_HPP_
#define OTSERVPP_PROTOCOLTRAITS_HPP_

#include "../forwarddcl.hpp"

namespace otservpp {

/*! Protocol configuration traits
 * A specialization of trait template ProtocolTraits can be provided for Connection
 * customization
 */
template <typename Protocol>
struct ProtocolTraits{
	/// A message/buffer type used to store incoming bytes from the remote peer
	typedef StandardInMessage IncomingMessage;

	/// A message/buffer type used to store outgoing bytes to the remote peer
	typedef StandardOutMessage OutgoingMessage;

	enum{
		/*! Should be set to the time in seconds this connection will wait on read operations
		 * before timing out
		 */
		ReadTimeOut  = 30,

		/*! Should be set to the time in seconds this connection will wait on write operations
		 * before timing out
		 */
		WriteTimeOut = 30  ///<
	};
};

} /* namespace otservpp */

#endif // OTSERVPP_PROTOCOLTRAITS_HPP_
