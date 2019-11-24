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

#ifndef PROPERTY_VALUE_PROVIDER_HPP
#define PROPERTY_VALUE_PROVIDER_HPP

#include <mox/metadata/variant.hpp>

namespace mox
{

class Property;

class AbstractPropertyValueProvider;
using PropertyValueProviderSharedPtr = std::shared_ptr<AbstractPropertyValueProvider>;

/// AbstractPropertyValueProvider class defines the interface for the property value providers in mox.
/// You can derive your custiom value providers by subclassing from this class and implement override
/// the desired functionalities. The minimum required from a proeprty value provider is to implement
/// the \l getLocalValue() and \l setLocalValue() methods.
///
/// Each property value is responsible to maintain the value it holds. The base class provides API to
/// detect value changes, so you don't need to worry about those when providing only the default set
/// of functionalities.
///
/// You must attach a property value provider to a property to be able to use it. After attached, you
/// can activate the value provider by calling the activate() method. You can attach a value provider
/// to a property by calling attach() method.
///
/// You can have multiple value providers attached to a property, however only one of those can provide
/// the value of that property. You can check from a value provider whether it is the active one by calling
/// isActive() method.
///
/// When a property is destroyed, it detaches all value providers. You can detach the value provider
/// manually by calling the detach() method.
class MOX_API AbstractPropertyValueProvider : public std::enable_shared_from_this<AbstractPropertyValueProvider>
{
public:
    /// Destructor.
    virtual ~AbstractPropertyValueProvider();

    /// Attach the property value provider to a \a property. On successfull attach, the value provider
    /// is also activated.
    /// \param property The property to attach a value provider.
    /// \throws Exception with ExceptionType::ValueProviderAlreadyAttached code if the value provider
    /// is already attached.
    void attach(Property& property);

    /// Detaches a property value provider. If the value provider is also the active one, deactivates
    /// it prior to detach it.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached code if the value provider
    /// is not attached.
    void detach();

    /// Returns the attaches state of the value provider.
    /// \return If the value provider is attached, returns \e true, otherwise \e false.
    bool isAttached() const;

    /// Activates a value provider.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached if the value provider is not attached.
    /// \throws Exception with ExceptionType::ActiveValueProvider if the value provider is already active.
    void activate();

    /// Deactivates the value provider.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached if the value provider is not attached.
    /// \throws Exception with ExceptionType::InactiveValueProvider if the value provider is not active.
    void deactivate();

    /// Returns the active state of a value provider.
    /// \return If the value provider is active, returns \e true, otherwise \e false.
    bool isActive() const;

    /// Returns the local value maintained by the value provider. You must implement this method in
    /// your value provider.
    /// \return The local value of the value provider stored in a variant.
    virtual Variant getLocalValue() const = 0;

    /// Sets the local value of a value provider. If the \a value passed as argument differs from the
    /// current one, the value provider emits the changed signal of the property.
    /// \param value The value to set to the value provider.
    void set(const Variant& value);

    /// The method restores the value provider's local value to the defaults. The method is called
    /// only on the property's default value provider.
    virtual void resetToDefault() {}

protected:
    /// Constructor.
    explicit AbstractPropertyValueProvider() = default;

    /// The method sets the local value and returns the local value changed state.
    /// \param value The new value to set for the local value.
    /// \return If the value is changes, returns \e true, otherwise returns \e false.
    virtual bool setLocalValue(const Variant& value) = 0;
    /// The method is called when the value provider attach operation is completed.
    virtual void onAttached() {}
    /// The method is called prior to the property gets detached.
    virtual void onDetached() {}
    /// The method is called when a value provider activation is completed.
    virtual void onActivated() {}
    /// The method is called when a value provider deactivation is completed.
    virtual void onDeactivated() {}

    /// The property to which the value provider is attached.
    Property* m_property = nullptr;
    /// The active state of the value provider.
    bool m_isActive = false;
};

} // namespace mox

#endif // PROPERTY_VALUE_PROVIDER_HPP
