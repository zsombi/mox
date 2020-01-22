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

#ifndef BINDING_GROUP_HPP
#define BINDING_GROUP_HPP

#include <mox/config/deftypes.hpp>
#include <vector>

namespace mox
{

class Property;
class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

class BindingGroup;
using BindingGroupSharedPtr = std::shared_ptr<BindingGroup>;

/// Groups bindings so they act like a single binding. The aim is to group bindings that are detached
/// from their targets whenever one of the binding sin the group is detached.
class MOX_API BindingGroup : public std::enable_shared_from_this<BindingGroup>
{
public:
    /// Destructor.
    ~BindingGroup();

    /// Adds a binding to the group.
    void addBinding(Binding& binding);

    /// Removes a binding from the group.
    void removeBinding(Binding& binding);

    /// Removes all bindings from the group.
    void ungroupBindings();

    /// Detaches the group by detaching all the bindings from the group.
    void detach();

    /// Checks whether the group is empty. A group is empty when there is no binding in the group.
    bool isEmpty() const;

    /// Returns the number of bindings in a group.
    size_t getBindingCount() const;

    /// Returns the binding at \a index. The method asserts if the group is empty.
    BindingSharedPtr operator[](size_t index);

    /// Creates an empty binding group.
    static BindingGroupSharedPtr create();

    /// Creates a binding group by adding property bindings built from the \a properties. The bindings
    /// created between the properties are permanent.
    template <typename... TProperty>
    static BindingGroupSharedPtr bindPermanent(TProperty&... properties)
    {
        std::array<Property*, sizeof...(TProperty)> propArray = {{&properties...}};
        return bindProperties({propArray.begin(), propArray.end()}, true, false);
    }

    /// Creates a binding group by adding property bindings built from the \a properties. The bindings
    /// created between the properties are discardable.
    template <typename... TProperty>
    static BindingGroupSharedPtr bindAutoDiscard(TProperty&... properties)
    {
        std::array<Property*, sizeof...(TProperty)> propArray = {{&properties...}};
        return bindProperties({propArray.begin(), propArray.end()}, false, false);
    }

    /// Creates a binding group by adding property bindings built from the \a properties. The bindings
    /// created between the properties are permanent, and circular.
    template <typename... TProperty>
    static BindingGroupSharedPtr bindPermanentCircular(TProperty&... properties)
    {
        std::array<Property*, sizeof...(TProperty)> propArray = {{&properties...}};
        return bindProperties({propArray.begin(), propArray.end()}, true, true);
    }

    /// Creates a binding group by adding property bindings built from the \a properties. The bindings
    /// created between the properties are discardable, and circular.
    template <typename... TProperty>
    static BindingGroupSharedPtr bindAutoDiscardCircular(TProperty&... properties)
    {
        std::array<Property*, sizeof...(TProperty)> propArray = {{&properties...}};
        return bindProperties({propArray.begin(), propArray.end()}, false, true);
    }

    /// Executes a \a function on every binding in the group.
    template <typename Function>
    void forEach(Function function)
    {
        std::for_each(m_bindings.begin(), m_bindings.end(), function);
    }

protected:
    /// Constructor.
    explicit BindingGroup() = default;

    /// Helper function, binds properties in the vector and adds them to the group.
    static BindingGroupSharedPtr bindProperties(const std::vector<Property*>& properties, bool permanent, bool circular);

    using BindingCollection = std::vector<BindingSharedPtr>;
    BindingCollection m_bindings;
};

} // mox

#endif // BINDING_GROUP_HPP
