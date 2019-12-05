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
#include <mox/metadata/metadata.hpp>
#include <mox/metadata/metatype.hpp>
#include <mox/metadata/metatype_descriptor.hpp>

TEST(MetaDataEnum, test_enumerate_metatypes)
{
    auto scanner = [](const auto& des)
    {
        std::cout << "Metatype id[" << int(des.id()) << "] " << des.name() << std::endl;
        return false;
    };
    mox::metadata::scanMetatypes(scanner);
}
TEST(MetaDataEnum, test_enumerate_metaclasses)
{
    auto scanner = [](const auto& mc)
    {
        std::cout << "MetaClass: " << mox::MetatypeDescriptor::get(mc.getMetaTypes().first).name() << std::endl;

        auto propertyVisitor = [](const auto property)
        {
            std::cout << "  Property: " << property->signature() << std::endl;
            return false;
        };
        mc.visitProperties(propertyVisitor);

        auto signalVisitor = [](const auto signal)
        {
            std::cout << "  Signal: [" << signal << "] " << signal->signature() << std::endl;
            return false;
        };
        mc.visitSignals(signalVisitor);

        auto methodVisitor = [](const auto method)
        {
            std::cout << "  Method: " << method->signature() << std::endl;
            return false;
        };
        mc.visitMethods(methodVisitor);
        return false;
    };
    mox::metadata::scanMetaClasses(scanner);
}
