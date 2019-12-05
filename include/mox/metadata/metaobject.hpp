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

#ifndef METAOBJECT_HPP
#define METAOBJECT_HPP

#include <mox/metadata/metaclass.hpp>
#include <mox/utils/locks.hpp>

namespace mox
{

/// Base class for the classes that provide standalone type reflection.
class MOX_API MetaObject : public ObjectLock
{
public:
    /// Constructor.
    explicit MetaObject();
    /// Destructor.
    virtual ~MetaObject();

    ClassMetaData(MetaObject)
    {
    };
};

} // namespace mox

#endif // METAOBJECT_HPP
