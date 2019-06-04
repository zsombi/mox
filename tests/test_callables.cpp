/*
 * Copyright (C) 2017-2019 bitWelder
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>
 */

#include "test_framework.h"
#include <mox/metadata/callable.hpp>
#include <mox/metadata/metatype_descriptor.hpp>

using namespace mox;

static bool invoked = false;
static Metatype functorMetaType = Metatype::Invalid;
struct TestFunctor;

class Callables : public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerTestType<std::reference_wrapper<int>>();
        functorMetaType = registerTestType<TestFunctor>();
        invoked = false;
    }
};

void testFunc()
{
    invoked = true;
}

void testFunc2(int)
{
    invoked = true;
}

void testRefFunc(int& v)
{
    v *= 11;
}

int testRetFunc()
{
    return 101;
}

int factorial(int value)
{
    if (value == 1)
    {
        return 1;
    }
    return factorial(value - 1) * value;
}

int sum(int a, int b)
{
    return a + b;
}

void* ptrFunc()
{
    return nullptr;
}

TEST_F(Callables, test_callable_return_types)
{
    Callable testFuncCallable(testFunc);
    EXPECT_EQ(Metatype::Void, testFuncCallable.returnType().type);

    Callable testRetFuncCallable(testRetFunc);
    EXPECT_EQ(Metatype::Int, testRetFuncCallable.returnType().type);

    Callable ptrFuncCallable(ptrFunc);
    EXPECT_EQ(Metatype::Void, ptrFuncCallable.returnType().type);
    EXPECT_TRUE(ptrFuncCallable.returnType().isPointer);
}

TEST_F(Callables, test_callable_arguments)
{
    Callable testFuncCallable(testFunc);
    EXPECT_EQ(0u, testFuncCallable.argumentCount());

    Callable testFunc2Callable(testFunc2);
    EXPECT_EQ(1u, testFunc2Callable.argumentCount());
    EXPECT_EQ(Metatype::Int, testFunc2Callable.argumentType(0u).type);
    EXPECT_FALSE(testFunc2Callable.argumentType(0u).isConst);
    EXPECT_FALSE(testFunc2Callable.argumentType(0u).isPointer);
    EXPECT_FALSE(testFunc2Callable.argumentType(0u).isReference);

    Callable summCallable(sum);
    EXPECT_EQ(2u, summCallable.argumentCount());
    EXPECT_EQ(Metatype::Int, summCallable.argumentType(0u).type);
    EXPECT_FALSE(summCallable.argumentType(0u).isConst);
    EXPECT_FALSE(summCallable.argumentType(0u).isPointer);
    EXPECT_FALSE(summCallable.argumentType(0u).isReference);
    EXPECT_EQ(Metatype::Int, summCallable.argumentType(1u).type);
    EXPECT_FALSE(summCallable.argumentType(1u).isConst);
    EXPECT_FALSE(summCallable.argumentType(1u).isPointer);
    EXPECT_FALSE(summCallable.argumentType(1u).isReference);
}

TEST_F(Callables, test_invoke_callable_function_no_args)
{
    invoked = false;
    Callable callable(testFunc);
    mox::invoke<void>(callable);
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_apply_callable_function_no_args)
{
    invoked = false;
    Callable callable(testFunc);
    callable.apply(Callable::Arguments());
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_invoke_callable_function_no_args_with_args)
{
    Callable callable(testFunc);

    invoked = false;
    mox::invoke<void>(callable, 10, 20.0f, "30");
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_apply_callable_function_no_args_with_args)
{
    Callable callable(testFunc);

    invoked = false;
    Callable::Arguments args;
    args.add(10).add(20.0f).add("30");
    callable.apply(args);
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_invoke_callable_function_one_arg)
{
    Callable callable(testFunc2);
    mox::invoke<void>(callable, 10);
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_apply_callable_function_one_arg)
{
    Callable callable(testFunc2);
    Callable::Arguments args;
    args.add(10);
    callable.apply(args);
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_invoke_function_one_arg_with_multiple_params)
{
    Callable callable(testFunc2);
    mox::invoke<void>(callable, 10, "alma");
    EXPECT_TRUE(invoked);

    EXPECT_THROW(mox::invoke<void>(callable, "alma", 10), std::bad_any_cast);
}

TEST_F(Callables, test_invoke_callable_with_args_using_no_arg_throws)
{
    Callable callable(factorial);

    EXPECT_THROW(mox::invoke<int>(callable), mox::Callable::invalid_argument);
}

TEST_F(Callables, test_invoke_callable_function_with_args_and_ret)
{
    Callable callable(factorial);

    int ret = mox::invoke<int>(callable, 3);
    EXPECT_EQ(6, ret);
}

TEST_F(Callables, test_apply_callable_function_with_args_and_ret)
{
    Callable callable(factorial);

    Callable::Arguments args;
    args.add(5);
    int ret = std::any_cast<int>(callable.apply(args));
    EXPECT_EQ(120, ret);
}


struct TestFunctor
{
    bool invoked;
    explicit TestFunctor()
        : invoked(false)
    {
    }

    void voidMethod()
    {
        invoked = true;
    }
    void voidMethod2(int)
    {
        invoked = true;
    }

    int retMethod()
    {
        return 1010;
    }
    int retMethodWithDefArg(int v = 100)
    {
        return v * 10;
    }

    int constRet() const
    {
        return 101;
    }
};

struct SecondLevel : public TestFunctor
{
};

TEST_F(Callables, test_callable_type)
{
    Callable func(testFunc);
    EXPECT_EQ(FunctionType::Function, func.type());
    EXPECT_FALSE(func.isConst());

    Callable method(&TestFunctor::retMethod);
    EXPECT_EQ(FunctionType::Method, method.type());
    EXPECT_FALSE(method.isConst());

    Callable constMethod(&TestFunctor::constRet);
    EXPECT_EQ(FunctionType::Method, constMethod.type());
    EXPECT_TRUE(constMethod.isConst());

    Callable lambda([]() {});
    EXPECT_EQ(FunctionType::Functor, lambda.type());
    EXPECT_TRUE(lambda.isConst());
}

TEST_F(Callables, test_method_ret_and_argument_types)
{
    Callable callable(&TestFunctor::voidMethod2);

    EXPECT_EQ(FunctionType::Method, callable.type());
    EXPECT_EQ(Metatype::Void, callable.returnType().type);
    EXPECT_EQ(1u, callable.argumentCount());
    EXPECT_EQ(Metatype::Int, callable.argumentType(0u).type);
    EXPECT_EQ(functorMetaType, callable.classType());
}

TEST_F(Callables, test_function_class_type_invalid)
{
    Callable callable(testFunc2);

    EXPECT_EQ(Metatype::Invalid, callable.classType());
}

TEST_F(Callables, test_invoke_method_no_arg)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::voidMethod);

    mox::invoke<void>(callable, functor);
    EXPECT_TRUE(functor.invoked);
}

TEST_F(Callables, test_apply_method_no_arg)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::voidMethod);

    Callable::Arguments args;
    callable.apply(functor, args);
    EXPECT_TRUE(functor.invoked);
}

TEST_F(Callables, test_invoke_method_one_arg)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::voidMethod2);

    mox::invoke<void>(callable, functor, 101);
    EXPECT_TRUE(functor.invoked);
}

TEST_F(Callables, test_apply_method_one_arg)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::voidMethod2);

    Callable::Arguments args(101);
    callable.apply(functor, args);
    EXPECT_TRUE(functor.invoked);
}

TEST_F(Callables, test_invoke_method_no_arg_ret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::retMethod);

    auto result = mox::invoke<int>(callable, functor);
    EXPECT_EQ(1010, result);
}

TEST_F(Callables, test_apply_method_no_arg_ret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::retMethod);

    auto result = callable.apply(functor, Callable::Arguments());
    EXPECT_EQ(1010, std::any_cast<int>(result));
}

TEST_F(Callables, test_invoke_method_default_arg_ret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::retMethodWithDefArg);

    // This fails as the formal arguments are not the same as the actual ones.
    auto result = mox::invoke<int>(callable, functor, 100);
    EXPECT_EQ(1000, result);
}

TEST_F(Callables, test_apply_method_default_arg_ret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::retMethodWithDefArg);

    // This fails as the formal arguments are not the same as the actual ones.
    Callable::Arguments args(100);
    auto result = callable.apply(functor, args);
    EXPECT_EQ(1000, std::any_cast<int>(result));
}

TEST_F(Callables, test_invoke_method_constret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::constRet);

    auto result = mox::invoke<int>(callable, functor);
    EXPECT_EQ(101, result);

    result = mox::invoke<int>(callable, functor, "monkey");
    EXPECT_EQ(101, result);
}

TEST_F(Callables, test_apply_method_constret)
{
    TestFunctor functor;
    Callable callable(&TestFunctor::constRet);

    auto result = callable.apply(functor, Callable::Arguments());
    EXPECT_EQ(101, std::any_cast<int>(result));

    result = callable.apply(functor, Callable::Arguments("monkey"));
    EXPECT_EQ(101, std::any_cast<int>(result));
}

TEST_F(Callables, test_lambda)
{
    auto lambda = []()
    {
        invoked = true;
    };
    Callable callable(lambda);
    EXPECT_FALSE(invoked);
    mox::invoke<void>(callable);
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_lambda_with_args)
{
    auto lambda = [](int, std::string)
    {
        invoked = true;
    };
    Callable callable(lambda);
    EXPECT_FALSE(invoked);
    EXPECT_THROW(mox::invoke<void>(callable), mox::Callable::invalid_argument);

    EXPECT_THROW(mox::invoke<void>(callable, 10, "alma"), std::bad_any_cast);
    EXPECT_FALSE(invoked);

    mox::invoke<void>(callable, 10, std::string("alma"));
    EXPECT_TRUE(invoked);
}

TEST_F(Callables, test_lambda_with_ret)
{
    auto lambda = [](size_t v, std::string s) -> size_t
    {
        return v * s.length();
    };
    Callable callable(lambda);
    auto result = invoke<size_t>(callable, 10ul, std::string("alma"));
    EXPECT_EQ(40u, result);
}

TEST_F(Callables, test_invoke_instance_with_function)
{
    Callable callable(factorial);
    TestFunctor f;

    EXPECT_THROW(invoke<int>(callable, f), std::bad_any_cast);
}

struct AnyClass
{
};

TEST_F(Callables, test_invoke_with_other_instance)
{
    Callable callable(&TestFunctor::voidMethod);
    AnyClass any;

    EXPECT_THROW(invoke<void>(callable, any, 10), std::bad_any_cast);
}


TEST_F(Callables, test_lambda_callables)
{
    Callable c1([](){});

    EXPECT_EQ(Metatype::Void, c1.returnType().type);
    EXPECT_EQ(0u, c1.argumentCount());

    Callable c2([](int) {});
    EXPECT_EQ(Metatype::Void, c2.returnType().type);
    EXPECT_EQ(1u, c2.argumentCount());
    EXPECT_EQ(Metatype::Int, c2.argumentType(0u).type);

    Callable c3([](int, std::string) {});
    EXPECT_EQ(Metatype::Void, c3.returnType().type);
    EXPECT_EQ(2u, c3.argumentCount());
    EXPECT_EQ(Metatype::Int, c3.argumentType(0u).type);
    EXPECT_EQ(Metatype::String, c3.argumentType(1u).type);

    Callable c4([]() -> int { return -1; });
    EXPECT_EQ(Metatype::Int, c4.returnType().type);
    EXPECT_EQ(0u, c4.argumentCount());

    Callable c5([](void*) -> void* { return nullptr; });
    EXPECT_EQ(Metatype::Void, c5.returnType().type);
    EXPECT_TRUE(c5.returnType().isPointer);
    EXPECT_EQ(1u, c5.argumentCount());
    EXPECT_EQ(Metatype::Void, c5.argumentType(0u).type);
    EXPECT_TRUE(c5.argumentType(0u).isPointer);
}

TEST_F(Callables, test_superclass_callable_invoked_with_derived_instance)
{
    SecondLevel l2;
    Callable callableL1(&TestFunctor::voidMethod);
    Callable callableL2(&SecondLevel::voidMethod);

    EXPECT_THROW(invoke<void>(callableL1, l2), std::bad_any_cast);
    EXPECT_THROW(invoke<void>(callableL2, l2), std::bad_any_cast);
}
