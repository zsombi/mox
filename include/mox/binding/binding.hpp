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

#ifndef BINDING_HPP
#define BINDING_HPP

#include <mox/property/property.hpp>

namespace mox
{

class Property;
class BindingGroup;
using BindingGroupSharedPtr = std::shared_ptr<BindingGroup>;

class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

/// Scoping the current binding. Used in subscribing bindings to the properties
/// present in a binding expression.
struct BindingScope
{
    static inline Binding* currentBinding = nullptr;
    explicit BindingScope(Binding& newCurrent)
        : backup(currentBinding)
    {
        currentBinding = &newCurrent;
    }
    ~BindingScope()
    {
        currentBinding = backup;
    }

private:
    Binding* backup = nullptr;
};

/// Enumeration representing the different states of a binding.
enum class BindingState
{
    /// The binding is in process of attaching to a target.
    Attaching,
    /// The binding is attached to a target.
    Attached,
    /// The binding is in process of detaching from a target.
    Detaching,
    /// The binding detached from a target.
    Detached,
    /// The binding is invalid, due to a source property present in the binding being unavailable.
    Invalid
};

class BindingPrivate;
/// Binding class. Provides the interface for bindings on properties. You can create your own binding
/// by deriving from this class, and implement the evaluate() method. In this method you give a value
/// to the property you attach this binding. The property a binding is attached is called the binding
/// target.
///
/// Mox has two types of bindings: permanent and discardable bindings. Permanent bindings survive
/// write operation on the target property, whereas discardable properties are detached from the target
/// property on write.
///
/// When a binding is disabled, the binding is excluded from the automatic evaluation. When a binding
/// is re-enabled, the binding restores the state of the target property to the state preserved by the
/// binding, by re-evaluating the binding. You can control this behavior by disabling the feature on
/// the binding. You can disable the feature by calling setEvaluateOnEnabled() method.
class MOX_API Binding : public std::enable_shared_from_this<Binding>
{
    DECLARE_PRIVATE(Binding)

public:
    /// Destructor.
    virtual ~Binding();

    /// Evaluates the binding. You can override this method to provide the value for the target of the
    /// binding.
    virtual void evaluate() = 0;

    /// Checks the validity of the binding. A binding is marked invalid if a property in the binding
    /// is destroyed, while the binding is still being attached.
    /// \return The valid state of the binding.
    bool isValid() const;

    /// Returns the attached state of the binding. A binding is attached when it has a target property.
    /// \return If the binding is attached, returns \e true, otherwise \e false.
    bool isAttached() const;

    /// Returns the state of a binding.
    /// \return The state of a binding.
    BindingState getState() const;

    /// Returns the permanent state of the binding.
    /// \return If the binding is permanent, returns \e true, otherwise \e false.
    bool isPermanent() const;

    /// Returns the enabled state of the binding.
    /// \return If the binding is permanent, returns \e true, otherwise \e false.
    bool isEnabled() const;

    /// Changes the enabled state of the binding.
    /// \param enabled The enabled state of the property.
    void setEnabled(bool enabled);

    /// Returns the state of the evaluate-on-enabled feature of the binding.
    /// \return If the evaluate-on-enabled is on, returns \e true, otherwise returns \e false.
    bool doesEvaluateOnEnabled() const;

    /// Changes the evaluate-on-enabled feature of the binding.
    /// \param doEvaluate If the feature is required, set \e true, otherwise set \e false.
    void setEvaluateOnEnabled(bool doEvaluate);

    /// Returns the target property of the binding.
    /// \return If the binding is attached, returns the target property of the binding. If the property is
    /// detached, returns \e nullptr.
    Property* getTarget() const;

    /// Detaches the binding from the target. If the binding is already detached, the calling this method
    /// has no effect.
    void detach();

    /// Returns the group where this binding belongs to. If the binding has no group, the binding is solitaire.
    /// \return The binding group of this binding.
    BindingGroupSharedPtr getBindingGroup();

protected:
    /// Constructor.
    explicit Binding(bool permanent);
    /// PIMPL constructor.
    explicit Binding(pimpl::d_ptr_type<BindingPrivate> dd);

    /// Overridable method called when the binding is attached to the target property.
    virtual void onAttached() {}
    /// Overridable method called when the binding is detached from the target.
    virtual void onDetached() {}
    /// Overridable method called when the binding's enabled state is changed.
    virtual void onEnabledChanged() {}

    pimpl::d_ptr_type<BindingPrivate> d_ptr;
};

} // mox

#endif // BINDING_HPP
