#include "staticadapterutil.hpp"
#include "../../scheduler.hpp"

OTSERVPP_STATIC_ADAPTER_T(DeferredTaskAdapter, otservpp::DeferredTask){
	using otservpp::DeferredTask;
	module(L)
	[
	 /*-- Represents the future execution of a function
		  You cannot create instances of this class directly, use @{callAfter} instead.
		  @type DeferredTask
	  */
	 class_<DeferredTask, otservpp::DeferredTaskPtr>("DeferredTask")
		 /*--Cancels the future execution of the bounded function.
			 If the bounded function has already been executed or canceled, nothing is done.
			 @function cancel
			 @ret boolean whether the execution was canceled
			 @usage
				 local task = callAfter(1000, function() print("nope") end)
				 task:cancel() -- nothing is printed
		  */
		 .def("cancel", &DeferredTask::cancel)
	];
}

OTSERVPP_STATIC_ADAPTER_T(IntervalTaskAdapter, otservpp::IntervalTask){
	using otservpp::IntervalTask;
	module(L)
	[
	 /*--Represents the continuous execution of a function
		 You cannot create instance of this class directly, use @{callEvery} instead.
		 @type IntervalTask
	  */
	 class_<IntervalTask>("IntervalTask")

	 	 /*--Cancels the execution of the bounded function
			Canceling an already canceled function does nothing.
			@function cancel
			@ret boolean whether the execution was canceled
			@usage
				local task
				task = callEvery(1000, function()
					print("yep")  -- prints yep only one time
					task:cancel()
				end)
		  */
		 .def("cancel", &IntervalTask::cancel)

		 /*--Restarts the execution interval
			The current execution interval is dropped and restarted, if the optional argument
			newInteral is given, it is used as the new interval.
			@function reschedule
			@p number newInterval Milliseconds
			@usage
				local task = callEvery(2500, function() print("i'm printed after 5s") end)
				task:reschedule(5000)
		  */
		 .def("reschedule", (void(IntervalTask::*)())&IntervalTask::reschedule)
		 .def("reschedule", (void(IntervalTask::*)(int))&IntervalTask::reschedule)
	];
}
