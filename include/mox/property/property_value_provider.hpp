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


/// AbstractPropertyValueProvider class is the base class for the property value providers in Mox.
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
class MOX_API AbstractPropertyValueProvider : public std::enable_shared_from_this<AbstractPropertyValueProvider>
{
public:
    /// Destructor.
    virtual ~AbstractPropertyValueProvider();

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

    /// Detaches a property value provider. If the value provider is also the active one, deactivates
    /// it prior to detach it.
    /// \throws Exception with ExceptionType::ValueProviderNotAttached code if the value provider
    /// is not attached.
    void detach();

    /// Returns the attaches state of the value provider.
    /// \return If the value provider is attached, returns \e true, otherwise \e false.
    bool isAttached() const;

    /// Returns local value of the property value provider. The local value is the default value of a
    /// property value provider.
    /// \return The local value of the property value provider, stored in a variant.
    virtual Variant getLocalValue() const = 0;

protected:
    /// Constructor.
    explicit AbstractPropertyValueProvider(ValueProviderFlags flags = ValueProviderFlags::Generic);

    /// Updates the property value without breaking any value provider.
    /// \param value The value to update.
    void update(const Variant& value);

    /// The method is called when the value provider attach operation is completed.
    virtual void onAttached() {}
    /// The method is called prior to the property gets detached.
    virtual void onDetached() {}

    /// The property to which the value provider is attached.
    Property* m_property = nullptr;
    /// The flags of the property value provider.
    const ValueProviderFlags m_flags = ValueProviderFlags::Generic;
};


/// Generic property value provider with default value storage.
/// \tparam ValueType The type of the value the value provider manages.
/// \tparam ValueProviderFlags The value provider flags.
template <typename ValueType, ValueProviderFlags ProviderFlags = ValueProviderFlags::Generic>
class PropertyValueProvider : public AbstractPropertyValueProvider
{
    ValueType m_default;

public:
    /// Constructor.
    explicit PropertyValueProvider(ValueType defaultValue)
        : AbstractPropertyValueProvider(ProviderFlags)
        , m_default(defaultValue)
    {
    }

    /// Override of AbstractPropertyValueProvider::getLocalValue().
    Variant getLocalValue() const override
    {
        return Variant(m_default);
    }

protected:
    /// Override of AbstractPropertyValueProvider::onAttached().
    void onAttached() override
    {
        update(Variant(m_default));
    }
};

} // namespace mox

#endif // PROPERTY_VALUE_PROVIDER_HPP
