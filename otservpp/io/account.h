#ifndef OTSERVPP_IO_ACCOUNT_H_
#define OTSERVPP_IO_ACCOUNT_H_

#include <stdint.h>
#include <string>
#include "../forwarddcl.hpp"
#include "../account.h"

namespace otservpp { namespace io {

class Account {
public:
	Account();

	template <class Handler>
	AccountPtr loadAccount(std::string, std::string, Handler)
	{
		return AccountPtr();
	}

	template <class Handler>
	int64_t loadAccountId(std::string, std::string, Handler)
	{
		return -1;
	}
};

} /* namespace io */
} /* namespace otservpp */

#endif // OTSERVPP_IO_ACCOUNT_H_
