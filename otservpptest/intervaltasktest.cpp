#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include "otservpp/scheduler.hpp"

using otservpp::IntervalTask;
using otservpp::IntervalTaskPtr;
using otservpp::makeIntervalTask;
using boost::system::error_code;

#define CALL_TIMES \
	for(int i = 0, intervalTime = 0; i < this->callIterations; ++i, intervalTime = this->baseTime*i)

#define INTERVAL_TIMES \
	for(int interval = 1; interval <= this->intervalIterations; ++interval)

namespace {
	int taskCalls;
	bool taskAborted;

	void taskImpl(const error_code& e)
	{
		if(!e)
			++taskCalls;
		else if(e == boost::asio::error::operation_aborted)
			taskAborted = true;
		else
			throw boost::system::system_error(e);
	}

	void taskImpl(const error_code& e, IntervalTask* task)
	{
		ASSERT_TRUE(task);
		taskImpl(e);
	}

	struct UnaryFreeFunctionTask{
		static IntervalTask::Unary func(){ return &taskImpl; }
	};

	struct BinaryFreeFunctionTask{
		static IntervalTask::Binary func(){ return &taskImpl; }
	};

	struct UnaryFunctorTask{
		void operator()(const error_code& e){ taskImpl(e); }
		static UnaryFunctorTask func(){ return {}; }
		char payload[100];
	};

	struct BinaryFunctorTask{
		void operator()(const error_code& e, IntervalTask* t){ taskImpl(e, t); }
		static BinaryFunctorTask func(){ return {}; }
		char payload[100];
	};

	struct UnaryStdFunctionTask{
		static std::function<std::remove_pointer<IntervalTask::Unary>::type> func() {
			return {UnaryFreeFunctionTask::func()}; }
	};

	struct BinaryStdFunctionTask{
		static std::function<std::remove_pointer<IntervalTask::Binary>::type> func() {
			return {BinaryFreeFunctionTask::func()}; }
	};

	typedef ::testing::Types<
		UnaryFreeFunctionTask, BinaryFreeFunctionTask,
		UnaryFunctorTask, BinaryFunctorTask,
		UnaryStdFunctionTask, BinaryStdFunctionTask>
	FunctionTypes;
}

template <class FunctionType>
class IntervalTaskTest : public ::testing::Test{
protected:
	void SetUp(){
		taskCalls = 0;
		taskAborted = false;
	}

	IntervalTaskPtr makeTask(int interval)
	{
		return makeIntervalTask(ioService, interval, FunctionType::func());
	}

	void runOne()
	{
		ASSERT_EQ(ioService.run_one(), 1);
		ioService.reset();
	}

	boost::asio::io_service ioService;
	int baseTime = 3;
	int intervalIterations = 3;
	int callIterations = 3;
};

TYPED_TEST_CASE(IntervalTaskTest, FunctionTypes);

TYPED_TEST(IntervalTaskTest, ExecutesInterval){
	CALL_TIMES{
		taskAborted = false;
		taskCalls = 0;
		auto task = this->makeTask(intervalTime);
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_EQ(interval, taskCalls);
		}
		ASSERT_TRUE(task->cancel());
		this->runOne();
		ASSERT_TRUE(taskAborted);
	}
}

TYPED_TEST(IntervalTaskTest, ExecutesIntervalIfItGoesOutOfScope){
	taskCalls = 0;
	{
		this->makeTask(this->baseTime);
	}
	INTERVAL_TIMES{
		this->runOne();
		ASSERT_EQ(interval, taskCalls);
	}
}

TYPED_TEST(IntervalTaskTest, CanRescheduleTaskAfterBeingCanceled){
	CALL_TIMES{
		taskAborted = false;
		taskCalls = 0;
		auto task = this->makeTask(intervalTime);
		this->runOne();
		ASSERT_EQ(taskCalls, 1);
		task->cancel();
		this->runOne();
		ASSERT_TRUE(taskAborted);
		taskAborted = false;
		taskCalls = 0;
		task->reschedule();
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_EQ(interval, taskCalls);
		}
		task->cancel();
		this->runOne();
		ASSERT_TRUE(taskAborted);
	}
}
