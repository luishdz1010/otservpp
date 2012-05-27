#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include "otservpp/scheduler.hpp"

#define CALL_TIMES \
	for(int i = 0, callTime = 0; i < this->callIterations; ++i, callTime = this->baseTime*i)

using otservpp::DeferredTask;
using otservpp::DeferredTaskPtr;
using otservpp::makeDeferredTask;
using boost::system::error_code;

namespace {
	bool taskCalled;
	bool taskAborted;

	void taskImpl(const error_code& e)
	{
		if(!e)
			taskCalled = true;
		else if(e == boost::asio::error::operation_aborted)
			taskAborted = true;
		else
			throw boost::system::system_error(e);
	}

	void taskImpl(const error_code& e, DeferredTask* task)
	{
		ASSERT_TRUE(task);
		taskImpl(e);
	}

	struct UnaryFreeFunctionTask{
		static DeferredTask::Unary func(){ return &taskImpl; }
	};

	struct BinaryFreeFunctionTask{
		static DeferredTask::Binary func(){ return &taskImpl; }
	};

	struct UnaryFunctorTask{
		void operator()(const error_code& e){ taskImpl(e); }
		static UnaryFunctorTask func(){ return {}; }
		char payload[100];
	};

	struct BinaryFunctorTask{
		void operator()(const error_code& e, DeferredTask* t){ taskImpl(e, t); }
		static BinaryFunctorTask func(){ return {}; }
		char payload[100];
	};

	struct UnaryStdFunctionTask{
		static std::function<std::remove_pointer<DeferredTask::Unary>::type> func() {
			return {UnaryFreeFunctionTask::func()}; }
	};

	struct BinaryStdFunctionTask{
		static std::function<std::remove_pointer<DeferredTask::Binary>::type> func() {
			return {BinaryFreeFunctionTask::func()}; }
	};

	typedef ::testing::Types<
		UnaryFreeFunctionTask, BinaryFreeFunctionTask,
		UnaryFunctorTask, BinaryFunctorTask,
		UnaryStdFunctionTask, BinaryStdFunctionTask>
	FunctionTypes;
}

template <class FunctionType>
class DeferredTaskTest : public ::testing::Test{
protected:
	void SetUp(){
		taskCalled = false;
		taskAborted = false;
	}

	DeferredTaskPtr makeTask(int callTime)
	{
		return makeDeferredTask(ioService, callTime, FunctionType::func());
	}

	void runOne()
	{
		ASSERT_EQ(ioService.run_one(), 1);
		ioService.reset();
	}

	boost::asio::io_service ioService;
	int baseTime = 3;
	int callIterations = 3;
};

TYPED_TEST_CASE(DeferredTaskTest, FunctionTypes);

TYPED_TEST(DeferredTaskTest, ExecutesTask){
	CALL_TIMES{
		taskCalled = false;
		this->makeTask(callTime);
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(DeferredTaskTest, ExecutesTaskIfItGoesOutOfScope){
	CALL_TIMES{
		taskCalled = false;
		{
			this->makeTask(callTime);
		}
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(DeferredTaskTest, AbortsTaskIfCanceledBeforeDeadline){
	CALL_TIMES{
		taskAborted = false;
		auto task = this->makeTask(callTime);
		ASSERT_TRUE(task->cancel());
		this->runOne();
		ASSERT_TRUE(taskAborted);
	}
}

TYPED_TEST(DeferredTaskTest, ExecutesTaskIfCanceledAfterDeadline){
	CALL_TIMES{
		taskCalled = false;
		auto task = this->makeTask(callTime);
		this->runOne();
		ASSERT_FALSE(task->cancel());
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(DeferredTaskTest, ExecutesNewTaskIfStartIsCalledAfterDeadline){
	CALL_TIMES{
		taskCalled = false;
		auto task = this->makeTask(callTime);
		this->runOne();
		ASSERT_TRUE(taskCalled);
		taskCalled = false;
		task->start(callTime, TypeParam::func());
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(DeferredTaskTest, ExecutesNewTaskAndAbortsOldIfStartIsCalledBeforeDeadline){
	CALL_TIMES{
		taskAborted = taskCalled = false ;
		auto task = this->makeTask(callTime);
		task->start(callTime, TypeParam::func());
		this->runOne();
		ASSERT_TRUE(taskAborted);
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}


