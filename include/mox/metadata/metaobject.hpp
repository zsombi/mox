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

namespace mox
{

/// Base class for the classes that provide standalone type reflection.
class MOX_API MetaObject
{
public:
    /// Destructor.
    virtual ~MetaObject();

    /// Returns the static metaclass of the metaobject.
    static const MetaClass* getStaticMetaClass();

    /// Returns the dynamic metaclass of the metaobject.
    virtual const MetaClass* getDynamicMetaClass() const;

protected:
    /// Constructor.
    explicit MetaObject();
};

} // namespace mox

#endif // METAOBJECT_HPP