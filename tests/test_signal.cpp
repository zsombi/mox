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
#include <mox/metadata/metaclass.hpp>
#include <mox/metadata/metamethod.hpp>
#include <mox/metadata/callable.hpp>

using namespace mox;

class SignalTestClass
{
public:
    MIXIN_METACLASS_BASE(SignalTestClass)
    {

    };
};

class SlotHolder
{
public:
    MIXIN_METACLASS_BASE(SlotHolder)
    {

    };
};

class SignalTest: public UnitTest
{
protected:
    void SetUp() override
    {
        UnitTest::SetUp();
        registerMetaType<SignalTestClass>();
        registerMetaType<SlotHolder>();
    }
};

TEST_F(SignalTest, test_connect_method)
{

}

TEST_F(SignalTest, test_connect_function)
{

}

TEST_F(SignalTest, test_connect_lambda)
{

}

TEST_F(SignalTest, test_connect_signal)
{

}

TEST_F(SignalTest, test_disconnect)
{

}

TEST_F(SignalTest, test_emit_signal)
{

}

TEST_F(SignalTest, test_disconnect_on_emit)
{

}

TEST_F(SignalTest, test_delete_connection)
{

}

TEST_F(SignalTest, test_delete_signal_source)
{

}

TEST_F(SignalTest, test_delete_connection_destination)
{

}

TEST_F(SignalTest, test_connect_in_emit)
{

}
