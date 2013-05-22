#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test_thread_pool
#include <boost/test/unit_test.hpp>

#include "thread_pool.hpp"

thread_pool pool;

// Tests with passing lambda functions to TP
BOOST_AUTO_TEST_CASE(test_lambda_without_return_value)
{
	int answer_to_question_of_life = 1;
	auto result = pool.execute_async([&answer_to_question_of_life](){ answer_to_question_of_life = 2; });
	result.get();
	BOOST_CHECK(answer_to_question_of_life == 2);
}

BOOST_AUTO_TEST_CASE(test_lambda_with_return_value)
{
	auto result = pool.execute_async([](){ return 42; });
	BOOST_CHECK(result.get() == 42);	
}

// Tests with passing C functions to TP
int g_answer_to_question_of_life = 0;
void answer_to_question_of_life() {
	g_answer_to_question_of_life = 42;
}

int return_answer_to_question_of_life() {
	return 42;
}

BOOST_AUTO_TEST_CASE(test_function_without_return_value)
{
	auto result = pool.execute_async(answer_to_question_of_life);
	result.get();
	BOOST_CHECK(g_answer_to_question_of_life == 42);
}

BOOST_AUTO_TEST_CASE(test_function_with_return_value)
{
	auto result = pool.execute_async(return_answer_to_question_of_life);
	BOOST_CHECK(result.get() == 42);	
}

// Tests with passing pointer to member function to TP
BOOST_AUTO_TEST_CASE(test_pointer_to_member_function_without_return_value)
{
	class oracle {
	public:
		int x;
		void compute_answer_to_question_of_life() {
			x += 42;
		}
	} robot;

	{
		robot.x = 0;
		auto result = pool.execute_async(&robot, &oracle::compute_answer_to_question_of_life);
		result.get();
		BOOST_CHECK(robot.x == 42);
	}

	//{
	//	robot.x = 1;
	//	auto result = pool.execute_async(robot, &oracle::compute_answer_to_question_of_life);
	//	result.get();
	//	assert(robot.x == 43);
	//}
}

BOOST_AUTO_TEST_CASE(test_pointer_to_member_function_with_return_value)
{
	class oracle {
	public:
		int answer_to_question_of_life() {
			return 42;
		}
	} robot;
	
	{
		auto result = pool.execute_async(&robot, &oracle::answer_to_question_of_life);
		BOOST_CHECK(result.get() == 42);	
	}
	//{
	//	auto result = pool.execute_async(robot, &oracle::answer_to_question_of_life);
	//	auto res = result.get();
	//	assert(res == 42);	
	//}
}
