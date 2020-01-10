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

class PropertyValueProvider;
using PropertyValueProviderSharedPtr = std::shared_ptr<PropertyValueProvider>;

/// Property value provider flags.
enum class ValueProviderFlags
{
    Generic = 0,
    /// The value provider is the default value provider of the property. Default value providers
    /// are attached to the property where the property is defined.
    Default = 1,
    /// The property value provider is the only value provider for the property
    /// that can update the value of the property. When set, values passed to the
    /// property setter are ignored.
    Exclusive = 2,
    /// The property value provider survives explicit property write operation.
    /// This flag is automatically set for the default value providers.
    KeepOnWrite = 4,
};
ENABLE_ENUM_OPERATORS(ValueProviderFlags)


/// PropertyValueProvider class is the base class for the property value providers in Mox.
/// You can derive your custom value providers by subclassing from this class and implement override
/// the desired functionalities.
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
class MOX_API PropertyValueProvider : public std::enable_shared_from_this<PropertyValueProvider>
{
public:
    /// Destructor.
    virtual ~PropertyValueProvider();

    /// Returns the flags of the value provider.
    ValueProviderFlags getFlags() const;
    /// Tests the existence of the flags on the value provider.
    bool hasFlags(ValueProviderFlags flags) const;

    /// Attach the property value provider to a \a property. On successfull attach, the value provider
    /// is also activated.
    /// \param property The property to attach a value provider.
    /// \throws Exception with ExceptionType::ValueProviderAlreadyAttached code if the value provider
    /// is already attached.
    void attach(Property& property);

    /// Detaches a property value provider. If the value provider is also the active one, disables
    /// it prior to detach it.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached code if the value provider
    /// is not attached.
    void detach();

    /// Returns the attaches state of the value provider.
    /// \return If the value provider is attached, returns \e true, otherwise \e false.
    bool isAttached() const;

    /// Returns the enabled state of the property value provider.
    /// \return If the value provider is enabled, returns \e true, otherwise \e false.
    bool isEnabled() const;
    /// Sets the property value provider enabled state.
    /// \param enabled The new enabled state of the value provider.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached code if the value provider
    /// is not attached.
    void setEnabled(bool enabled);

    /// Returns local value of the property value provider. The local value is the default value of a
    /// property value provider.
    /// \return The local value of the property value provider, stored in a variant.
    virtual Variant getLocalValue() const;

protected:
    /// Constructor.
    explicit PropertyValueProvider(ValueProviderFlags flags = ValueProviderFlags::Generic);

    /// The activate is called when the property value provider is attached. Value providers
    /// can set initial value to the property they are attached to. The activation tries to
    /// enable the value provider.
    void activate();

    /// Updates the property value without removing value providers.
    /// \param value The value to update.
    void update(const Variant& value);

    /// The method is called when the value provider is attached to a property.
    virtual void onAttached() {}
    /// The method is called before the value provider is detached from a property.
    virtual void onDetached() {}

    /// The method is called when a value provider is attached with success, and prior it is
    /// enabled. Youcan override this method to provide value provider specific initial value
    /// for the attached property.
    virtual void onActivating() {}

    /// The method is called when the enabled state is changed. You can override this to set
    /// a value provider specific value to the attached property each time the value provider
    /// is enabled, or to back up th evalue provider's value when disabled, so you can restore
    /// it when it gets re-enabled.
    virtual void onEnabledChanged() {}

    // TODO: move to pimpl
    enum State
    {
        Attaching,
        Attached,
        Detaching,
        Detached
    };

    friend struct PropertyPrivate;
    friend class Property;

    PropertyValueProviderSharedPtr prev;
    PropertyValueProviderSharedPtr next;

    /// The property to which the value provider is attached.
    Property* m_property = nullptr;
    /// The flags of the property value provider.
    const ValueProviderFlags m_flags = ValueProviderFlags::Generic;
    /// Value provider state.
    State m_state = Detached;
    /// The enabled state.
    bool m_enabled = false;
};


/// Property value provider template for default property values.
template <typename ValueType, ValueProviderFlags ProviderFlags = ValueProviderFlags::Default>
class DefaultValueProvider : public PropertyValueProvider
{
    ValueType m_defaultValue;

public:
    /// Constructor.
    explicit DefaultValueProvider(ValueType defaultValue)
        : PropertyValueProvider(ProviderFlags)
        , m_defaultValue(defaultValue)
    {
    }

    /// Override of PropertyValueProvider::getLocalValue().
    Variant getLocalValue() const override
    {
        return Variant(m_defaultValue);
    }

protected:
    /// Override of PropertyValueProvider::onActivating().
    void onActivating() override
    {
        update(Variant(m_defaultValue));
    }
};

} // namespace mox

#endif // PROPERTY_VALUE_PROVIDER_HPP
