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
#include <mox/core/meta/core/metadata.hpp>
#include <mox/core/meta/core/metatype.hpp>
#include <mox/core/meta/core/metatype_descriptor.hpp>

TEST(MetaDataEnum, test_enumerate_default_metatypes)
{
    // Test the predefined types.
    std::array<std::string, int(mox::Metatype::UserType)> typeNames = {
        "void"s, "bool"s, "char"s, "byte"s, "short"s, "word"s, "int"s, "uint"s, "int64"s, "uint64"s,
        "float"s, "double"s, "std::string"s, "literal"s, "void*"s, "byte*"s, "int*"s, "int64*"s,
        "vector<int32>"s
    };
    auto it = typeNames.begin();
    auto scanner = [&it, &typeNames](const auto& des)
    {
        EXPECT_EQ(*it, des.name());
        ++it;
        return it == typeNames.end();
    };
    mox::metadata::findMetatype(scanner);
}
