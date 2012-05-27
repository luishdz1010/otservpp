#ifndef OTSERVPP_SCRIPT_SCHEDULER_H_
#define OTSERVPP_SCRIPT_SCHEDULER_H_

#include "adapterutil.hpp"
#include "../../scheduler.hpp"

OTSERVPP_NAMESPACE_SCRIPT_BEGIN

class SchedulerAdapter : public GlobalInstanceAdapter<otservpp::Scheduler>{
public:
	SchedulerAdapter(otservpp::Scheduler& s) :
		GlobalInstanceAdapter<otservpp::Scheduler>("[callAfter(), callEvery()]"),
		scheduler(s)
	{}

	void apply(lua_State* L) final override;

private:
	otservpp::Scheduler& scheduler;
};

OTSERVPP_NAMESPACE_SCRIPT_END

#endif // OTSERVPP_SCRIPT_SCHEDULER_H_
