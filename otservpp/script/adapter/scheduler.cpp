#include "scheduler.h"
#include <luabind/tag_function.hpp>

using namespace luabind;

OTSERVPP_NAMESPACE_SCRIPT_BEGIN

void SchedulerAdapter::apply(lua_State* L){
	module(L)
	[
	 /*--Calls func after ms milliseconds from now.
		@function callAfter
		@p number ms milliseconds to wait before calling func
		@p function func the function to be called
		@ret DeferredTask a handle to the task future execution
	 */
	 def("callAfter", [this](int ms, const object& func){
		return scheduler.callAfter(ms, [func]{ call_function<void>(func); });
	 }),

	 /*--Calls func every interval milliseconds.
		@function callEvery
		@p interval Milliseconds to wait between calls to func
		@p func The function to be called
		@ret IntervalTask a handle to the continuous execution
	 */
	 def("callEvery", [this](int ms, const object& func) {
		return scheduler.callEvery(ms, [func]{ call_function<void>(func); });
	 })
	];
}

OTSERVPP_NAMESPACE_SCRIPT_END
