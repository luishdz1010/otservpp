#include <gtest/gtest.h>
#include "otservpp/scheduler.hpp"

#define CALL_TIMES \
	for(int i = 0, callTime = 0; i < this->callIterations; ++i, callTime = this->baseTime*i)

#define INTERVAL_TIMES \
	for(int interval = 1; interval <= this->intervalIterations; ++interval)

using otservpp::Scheduler;

namespace{
	bool taskCalled;
	int taskCalls;

	struct EvaluatesTo{
		EvaluatesTo(bool v_) :
			v(new bool(v_))
		{}

		EvaluatesTo(const EvaluatesTo& other) :
			v(other.v)
		{}

		EvaluatesTo& operator=(bool nv){
			*v = nv;
			return *this;
		}

		std::shared_ptr<bool> v;
		explicit operator bool() const { return *(v.get()); }
	};

	void taskImpl()
	{
		taskCalled = true;
		++taskCalls;
	}

	struct FreeFunctionTask{
		static Scheduler::Task func(){ return &taskImpl; }
	};

	struct FunctorTask{
		void operator()(){ taskImpl(); }
		static FunctorTask func(){ return {}; }
		char payload[100];
	};

	struct StdFunctionTask{
		static std::function<void()> func() { return {&taskImpl}; }
	};

	typedef ::testing::Types<FreeFunctionTask, FunctorTask, StdFunctionTask> FunctionTypes;
}

template <class FunctionType>
class SchedulerTest : public ::testing::Test{
protected:
	SchedulerTest() {}

	void SetUp()
	{
		taskCalled = false;
		taskCalls = 0;
	}

	void runOne()
	{
		ASSERT_EQ(ioService.run_one(), 1);
		ioService.reset();
	}

	boost::asio::io_service ioService;
	Scheduler scheduler {ioService};
	int baseTime = 3;
	int intervalIterations = 3;
	int callIterations = 3;
};

TYPED_TEST_CASE(SchedulerTest, FunctionTypes);

TYPED_TEST(SchedulerTest, ExecutesTaskLater){
	taskCalled = false;
	this->scheduler.callLater(TypeParam::func());
	this->runOne();
	ASSERT_TRUE(taskCalled);
}

TYPED_TEST(SchedulerTest, ExecutesTaskLaterIfItGoesOutOfScope){
	taskCalled = false;
	{
		this->scheduler.callLater(TypeParam::func());
	}
	this->runOne();
	ASSERT_TRUE(taskCalled);
}

TYPED_TEST(SchedulerTest, ExecutesTaskAfter){
	CALL_TIMES{
		taskCalled = false;
		this->scheduler.callAfter(callTime, TypeParam::func());
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(SchedulerTest, ExecutesTaskAfterIfItGoesOutOfScope){
	CALL_TIMES{
		taskCalled = false;
		{
			this->scheduler.callAfter(callTime, TypeParam::func());
		}
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(SchedulerTest, ExecutesTaskAfterIfConditionTrue){
	CALL_TIMES{
		taskCalled = false;
		this->scheduler.callAfterIf(callTime, EvaluatesTo{true}, TypeParam::func());
		this->runOne();
		ASSERT_TRUE(taskCalled);
	}
}

TYPED_TEST(SchedulerTest, DoesNotExecutesTaskAfterIfConditionFalse){
	CALL_TIMES{
		taskCalled = false;
		this->scheduler.callAfterIf(callTime, EvaluatesTo{false}, TypeParam::func());
		this->runOne();
		ASSERT_FALSE(taskCalled);
	}
}

TYPED_TEST(SchedulerTest, ExecutesTaskEvery){
	CALL_TIMES{
		taskCalls = 0;
		auto task = this->scheduler.callEvery(callTime, TypeParam::func());
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_EQ(interval, taskCalls);
		}
		task->cancel();
		this->runOne();
	}
}

TYPED_TEST(SchedulerTest, ExecutesTaskEveryIfItGoesOutOfScope){
	taskCalls = 0;
	{
		this->scheduler.callEvery(this->baseTime, TypeParam::func());
	}
	INTERVAL_TIMES{
		this->runOne();
		ASSERT_EQ(interval, taskCalls);
	}
}

TYPED_TEST(SchedulerTest, ExecutesOrNotTaskEveryIfStatefulCondition){
	CALL_TIMES{
		taskCalls = 0;
		EvaluatesTo condition {true};
		auto task = this->scheduler.callEveryIf(callTime, condition, TypeParam::func());
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_EQ(interval, taskCalls);
		}
		condition = false;
		taskCalled = false;
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_FALSE(taskCalled);
		}
		condition = true;
		taskCalls = 0;
		INTERVAL_TIMES{
			this->runOne();
			ASSERT_EQ(interval, taskCalls);
		}
		task->cancel();
		this->runOne();
	}
}
