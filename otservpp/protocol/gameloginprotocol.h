#ifndef OTSERVPP_GAMELOGINPROTOCOL_H_
#define OTSERVPP_GAMELOGINPROTOCOL_H_

#include "basicloginprotocol.h"

namespace otservpp {

class GameLoginProtocol : public BasicLoginProtocol<GameLoginProtocol>{
public:
	GameLoginProtocol();

	static const char* getName()
	{
		return "game login protocol";
	}

	void handleFirstMessage(StandardInMessage& msg);

private:

};

} /* namespace otservpp */

#endif // OTSERVPP_GAMELOGINPROTOCOL_H_
