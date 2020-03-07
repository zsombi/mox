/*
 * Copyright (C) 2017-2020 bitWelder
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

#ifndef METABASE_P_HPP
#define METABASE_P_HPP

#include <mox/meta/metabase/metabase.hpp>
#include <property_p.hpp>

namespace mox
{

class SignalStorage;

class MetaBasePrivate
{
    using PropertyCollection = std::map<const PropertyType*, PropertyStorage*>;
    using SignalCollection = std::map<const SignalType*, SignalStorage*>;
    using DynamicPropertyContainer = std::vector<DynamicPropertyPtr>;

    SignalCollection signals;
    PropertyCollection properties;
    DynamicPropertyContainer dynamicProperties;

    void invalidateDynamicProperties();

public:
    DECLARE_PUBLIC_PTR(MetaBase)

    explicit MetaBasePrivate(MetaBase& pp)
        : p_ptr(&pp)
    {
    }

    void addSignal(SignalStorage& storage);
    void removeSignal(SignalStorage* storage);

    void addProperty(PropertyStorage& storage);
    void addDynamicProperty(DynamicPropertyPtr property);
    void removePropertyStorage(PropertyStorage* storage);

};

}

#endif // METABASE_P_HPP
