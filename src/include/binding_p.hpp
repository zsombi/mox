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

#ifndef BINDING_P_HPP
#define BINDING_P_HPP

#include <mox/core/meta/property/property.hpp>
#include <mox/core/meta/property/binding/binding.hpp>
#include <mox/core/meta/property/binding/binding_normalizer.hpp>
#include <mox/core/meta/property/binding/property_binding.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/utils/ref_counted.hpp>

#include <unordered_set>

namespace mox
{

class BindingPrivate : public RefCounted<size_t>
{
public:
    DECLARE_PUBLIC(Binding, BindingPrivate)

    using Base = RefCounted<size_t>;

    explicit BindingPrivate(Binding* pp, bool permanent);
    ~BindingPrivate();

    void addDependency(Property& dependency);
    void removeDependency(Property& dependency);
    void clearDependencies();
    void invalidate();

    inline void setGroup(BindingGroupSharedPtr grp)
    {
        group = grp;
    }
    inline void setEnabled(bool enabled)
    {
        this->enabled = enabled;
    }

protected:
    using Collection = std::unordered_set<Property*>;

    Collection dependencies;
    Binding* p_ptr = nullptr;
    BindingGroupSharedPtr group;
    Property* target = nullptr;
    BindingState state = BindingState::Detached;
    bool enabled:1;
    bool evaluateOnEnabled:1;
    bool isPermanent:1;

    friend class BindingLoopDetector;
};

class PropertyBindingPrivate : public BindingPrivate
{
public:
    DECLARE_PUBLIC(PropertyBinding, PropertyBindingPrivate)

    explicit PropertyBindingPrivate(PropertyBinding* pp, Property& source, bool permanent);

    Property* source = nullptr;
};

class BindingLoopDetector : public RefCounter<BindingPrivate>
{
    using BaseClass = RefCounter<BindingPrivate>;

    static inline BindingLoopDetector* last = nullptr;
    BindingLoopDetector* prev = nullptr;

public:
    explicit BindingLoopDetector(BindingPrivate& binding);
    ~BindingLoopDetector();
    bool tryNormalize(Variant& value);
    static BindingLoopDetector* getCurrent()
    {
        return last;
    }
};

}

#endif // BINDING_P_HPP
