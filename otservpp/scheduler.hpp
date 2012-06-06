#ifndef OTSERVPP_SCHEDULER_HPP_
#define OTSERVPP_SCHEDULER_HPP_

#include <functional>
#include <boost/thread/thread.hpp>
#include "forwarddcl.hpp"
#include "networkdcl.hpp"
#include "lambdautil.hpp"

namespace otservpp {

/// Base class for different DeferredTask classes, never use this directly
class BaseDeferredTask{
public:

	/* Executes the bounded task with an operation_aborted error code.
	 * \return true if the task was canceled, false if the task was already dispatched or
	 * interrupting it is simply not possible
	 */
	bool cancel()
	{
		return timer.cancel() > 0;
	}

	BaseDeferredTask(BaseDeferredTask&) = delete;
	void operator=(BaseDeferredTask&) = delete;

protected:
	explicit BaseDeferredTask(boost::asio::io_service& ioService) :
		timer(ioService)
	{}

	~BaseDeferredTask() = default;

	// these overloads provide dispatching for tasks with different arities, while keeping
	// a shared_ptr alive in the async_wait
	template <class Func, class TaskWrapperPtr>
	typename std::enable_if<function_traits<Func>::arity == 1>::type
	schedule(int ms, const TaskWrapperPtr& sthis, Func&& fn)
	{
		expiresFromNow(ms);
		timer.async_wait([sthis, fn](const SystemErrorCode& e) mutable { fn(e); });
	}

	template <class Func, class TaskWrapperPtr>
	typename std::enable_if<function_traits<Func>::arity == 2>::type
	schedule(int ms, const TaskWrapperPtr& sthis, Func&& fn)
	{
		expiresFromNow(ms);
		auto dthis = sthis.get();
		timer.async_wait([dthis, sthis, fn](const SystemErrorCode& e) mutable { fn(e, dthis); });
	}

	void expiresFromNow(int ms)
	{
		timer.expires_from_now(boost::posix_time::millisec(ms));
	}

	boost::asio::deadline_timer timer;
};

/*! Scheduling of tasks in a one-liner
 * This class is more helpful when used indirectly by calling one of the Scheduler methods.
 * The deferred task will be executed when the given time expires, after that, if there aren't
 * more objects holding a reference to the same task, it is deleted.
 *
 * The signature of every task must be either void task(const SystemErrorCode& e) or
 * void task(const SystemErrorCode& e, DeferredTask* _this).
 */
class DeferredTask :
	public BaseDeferredTask, public std::enable_shared_from_this<DeferredTask>{
public:
	typedef void(*Unary)(const SystemErrorCode&);
	typedef void(*Binary)(const SystemErrorCode&, DeferredTask*);

	template <class... Args>
	friend DeferredTaskPtr makeDeferredTask(boost::asio::io_service& ioService, Args&&... args);

	explicit DeferredTask(boost::asio::io_service& ioService) :
		BaseDeferredTask(ioService)
	{}

	/// Schedules the execution of the bounded task, if there was another task pending for
	/// execution it is immediately executed with an operation_aborted error code
	template <class Func>
	void start(int millisec, Func&& func)
	{
		schedule(millisec, shared_from_this(), std::forward<Func>(func));
	}
};

template <class... Args>
inline DeferredTaskPtr makeDeferredTask(boost::asio::io_service& ioService, Args&&... args)
{
	auto task = std::make_shared<DeferredTask>(ioService);
	task->start(std::forward<Args>(args)...);
	return task;
}

/*! Continuous scheduling of a task
 * As with DeferredTask, this class is better used by using a Scheduler directly. The given
 * task will be executed continuously every millisec milliseconds until its canceled,
 * interrupted by a thrown exception or by an asio::io_service error. After the continuous
 * execution is stopped, the IntervalTask will deleted provided there's no object holding
 * a reference to it.
 */
class IntervalTask :
	public BaseDeferredTask, public std::enable_shared_from_this<IntervalTask>{
public:
	typedef void(*Unary)(const SystemErrorCode&);
	typedef void(*Binary)(const SystemErrorCode&, IntervalTask*);
	typedef std::function<std::remove_pointer<Binary>::type> TaskStorage;


	/// Constructs an IntervalTask with the given millisec interval and task in the form:
	/// void task(const SystemErrorCode& e, IntervalTask*)
	template <class Func, class std::enable_if<function_traits<Func>::arity == 2, int>::type = 0>
	IntervalTask(boost::asio::io_service& ioService, int millisec, Func&& func) :
		BaseDeferredTask(ioService),
		ms(millisec),
		// interval logic
		fn([func](const SystemErrorCode& e, IntervalTask* this_) mutable {
			if(!e || e != boost::asio::error::operation_aborted)
				this_->reschedule();
			func(e, this_);
		})
	{}

	/// Constructs an IntervalTask with the given millisec interval and task in the form:
	/// void task(const SystemErrorCode& e)
	template <class Func, class std::enable_if<function_traits<Func>::arity == 1, int>::type = 0>
	IntervalTask(boost::asio::io_service& ioService, int millisec, Func&& func) :
		IntervalTask(ioService, millisec,
			[func](const SystemErrorCode& e, IntervalTask*) mutable { func(e); })
	{}

	/// Starts the execution of the bounded task
	void start()
	{
		reschedule();
	}

	/*! Restarts the scheduling of the bounded task
	 * This function is automatically called after the bounded task is executed in order to keep
	 * the execution interval.
	 * If you call this function manually, the current task will be executed with an
	 * operation_aborted error and restarted with the current time interval, if the task is
	 * expired (i.e. by a thrown exception or a call to IntervalTask::cancel) it is re-enabled.
	 */
	void reschedule()
	{
		schedule(ms, shared_from_this(), fn);
	}

	/// Does the same as the nularity version but it also changes the current interval
	void reschedule(int millisec)
	{
		ms = millisec;
		reschedule();
	}

private:
	int ms;
	TaskStorage fn;
};

template <class... Args>
inline IntervalTaskPtr makeIntervalTask(Args&&... args)
{
	auto task = std::make_shared<IntervalTask>(std::forward<Args>(args)...);
	task->start();
	return task;
}



/*! Convenience class for task scheduling
 * This class implements useful functions for dealing with scheduling. The signature of every
 * task must be:
 * 		void task();
 */
class Scheduler {
	/*! Helper class for dispatching parallel tasks
	 * It uses an internal io_service and a group of threads that take care of executing the
	 * given tasks. Every io_service shares the same ParallelExecutionService service, this
	 * is all handled by boost::asio.
	 */
	class ParallelExecutionService :
		public boost::asio::io_service::service{
	public:
		static boost::asio::io_service::id id;

		ParallelExecutionService(boost::asio::io_service& ioService) :
			boost::asio::io_service::service(ioService),
			dummyWork(boost::asio::io_service::work(workerIoService))
		{
			for(int i = 0, max = boost::thread::hardware_concurrency(); i < max; ++i)
				threadPool.create_thread(
						boost::bind(&boost::asio::io_service::run, &workerIoService));
		}

		void shutdown_service() override {}

		~ParallelExecutionService()
		{
			workerIoService.stop();
			threadPool.join_all();
		}

		template <class Task>
		void post(Task&& task)
		{
			workerIoService.post(std::forward<Task>(task));
		}

	private:
		boost::asio::io_service workerIoService;
		boost::asio::io_service::work dummyWork;
		boost::thread_group threadPool;
	};

public:
	typedef void(*Task)();

	Scheduler(boost::asio::io_service& ioService_) :
		ioService(ioService_),
		parallelService(boost::asio::use_service<ParallelExecutionService>(ioService_))
	{}

	Scheduler(const Scheduler&) = default;

	template <class Func>
	void callLater(Func&& fn)
	{
		ioService.post(std::forward<Func>(fn));
	}

	template <class Func>
	DeferredTaskPtr callAfter(int millisec, Func&& fn)
	{
		return makeDeferred(millisec, std::forward<Func>(fn));
	}

	template <class Check, class Func>
	DeferredTaskPtr callAfterIf(int millisec, Check&& check, Func&& fn)
	{
		return makeDeferred(millisec, [check, fn]() mutable { if(check) fn(); });
	}

	template <class Func>
	IntervalTaskPtr callEvery(int millisec, Func&& fn)
	{
		return makeInterval(millisec, std::forward<Func>(fn));
	}

	template <class Check, class Func>
	IntervalTaskPtr callEveryIf(int millisec, Check&& check, Func&& fn)
	{
		return makeInterval(millisec, [check, fn]() mutable { if(check) fn(); });
	}

	/// Executes fn in a worker thread
	template <class Func>
	void callInParallel(Func&& fn)
	{
		parallelService.post(std::forward<Func>(fn));
	}

	/// Executes fn in a worker thread and calls callback in the caller thread when finished
	template <class Func, class Callback>
	void callInParallel(Func&& fn, Callback&& callback)
	{
		auto& mainThread = ioService;
		parallelService.post([&mainThread, fn, callback]{
			fn();
			mainThread.post(callback);
		});
	}

private:
	template <class Func>
	struct Wrapper{
		Wrapper(Func&& f) :
			func(std::forward<Func>(f))
		{}

		void operator()(const SystemErrorCode& e)
		{
			if(!e)
				func();
			else if(e != boost::asio::error::operation_aborted)
				throw boost::system::system_error(e);
		}

		static_assert(std::is_same<typename std::remove_reference<Func>::type, Func>::value,
				"Is this always true?");

		Func func;
	};

	template <class Func>
	DeferredTaskPtr makeDeferred(int ms, Func&& task)
	{
		return makeDeferredTask(ioService, ms, Wrapper<Func>(std::forward<Func>(task)));
	}

	template <class Func>
	IntervalTaskPtr makeInterval(int ms, Func&& task)
	{
		return makeIntervalTask(ioService, ms, Wrapper<Func>(std::forward<Func>(task)));
	}

	boost::asio::io_service& ioService;
	ParallelExecutionService& parallelService;
};

} /* namespace otservpp */

#endif // OTSERVPP_SCHEDULER_HPP_
