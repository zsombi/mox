// Copyright (C) 2020 bitWelder

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <mox/core/metakernel/property_core.hpp>
#include <mox/core/metakernel/signals.hpp>

namespace mox { namespace metakernel {

/******************************************************************************
 * Templates
 */

// Forward declarations
template <class Type>
class Property;

template <class Type>
class PropertyBinding : public BindingCore
{
    Property<Type>& m_target;
    Property<Type>& m_source;

    ConnectionPtr m_sourceWatch;

public:
    static BindingPtr bind(Property<Type>& target, Property<Type>& source)
    {
        auto binding = make_polymorphic_shared_ptr<BindingCore>(new PropertyBinding<Type>(target, source));
        binding->attachToTarget(target);
        return binding;
    }

    Type evaluate() const
    {
        FATAL(m_isEnabled, "Cannot evaluate disabled bindings");
        return m_source;
    }

protected:
    explicit PropertyBinding(Property<Type>& target, Property<Type>& source)
        : m_target(target)
        , m_source(source)
    {
        m_sourceWatch = m_source.changed.connect(*this, &PropertyBinding<Type>::onSourceChanged);
    }
    ~PropertyBinding()
    {
    }

    void detachOverride() override
    {
        if (m_sourceWatch && m_sourceWatch->isConnected())
        {
            m_sourceWatch->disconnect();
            m_sourceWatch.reset();
        }
    }

    void onSourceChanged(Type value)
    {
        if (isEnabled())
        {
            ScopeSignalBlocker blockSource(m_source.changed);
            m_target = value;
        }
    }
};


/// The template class defines a read-write property with a change signal. Provides a default
/// property data provider.
template <class Type>
class Property : public PropertyCore
{
    // Default property data provider.
    class PropertyData : public Data
    {
        Type m_data;

    public:
        PropertyData(const Type& defaultValue)
            : Data(PropertyType::ReadWrite)
            , m_data(defaultValue)
        {
            set(defaultValue);
        }
        ArgumentData get() const override
        {
            return m_data;
        }
        void set(const ArgumentData& data) override
        {
            m_data = data;
        }
        bool isEqual(const ArgumentData &other) override
        {
            return m_data == Type(other);
        }
    };

public:
    /// The changed signal of the property.
    metakernel::Signal<Type> changed;

    /// Constructs a property with the default property value provider.
    explicit Property(const Type& defaultValue = Type())
        : PropertyCore(*(new PropertyData(defaultValue)), changed)
    {
        FATAL(getDataProvider().propertyType == PropertyType::ReadWrite, "The data provider is not meant for a writable property.");
    }

    /// Constructs a property with a custom property value provider.
    explicit Property(Data& dataProvider)
        : PropertyCore(dataProvider, changed)
    {
        FATAL(getDataProvider().propertyType == PropertyType::ReadWrite, "The data provider is not meant for a writable property.");
    }

    ~Property()
    {
        auto dp = dynamic_cast<PropertyData*>(&getDataProvider());
        if (dp)
        {
            delete dp;
        }
    }

    /// Property getter.
    operator Type() const
    {
        return static_cast<Type>(getDataProvider().get());
    }

    /// Property setter.
    void operator=(const Type& value)
    {
        auto& dp = getDataProvider();
        if (!dp.isEqual(value))
        {
            dp.set(value);
            changed(value);
        }
    }

    /// \name Bindings
    /// \{
    auto bind(Property<Type>& source)
    {
        // source to target
        auto binding1 = PropertyBinding<Type>::bind(*this, source);

        // target to source
        auto binding2 = PropertyBinding<Type>::bind(source, *this);

        // group the bindings
        auto group = make_polymorphic_shared<BindingCore, BindingGroup>();
        group->addToGroup(*binding1);
        group->addToGroup(*binding2);
//        auto group = make_polymorphic_shared<BindingCore, BindingGroup>(binding1, binding2);

        // Evaluate the binding.
        *this = Type(source);

        // return the group as the created binding
        return group;
    }
    /// \}
};


/// This template class provides functionalities for read-only properties. Unlike the writable
/// properties, the read-only properties use value providers. These value providers can update
/// the value of the property, so the users of the properties can get notified about the change
/// of the property value. A typical use case of these types of properties is to provide status
/// updates on an entity.
template <class Type>
class StatusProperty : public PropertyCore
{
public:
    /// The changed signal of the property.
    metakernel::Signal<Type> changed;

    explicit StatusProperty(Data& dataProvider)
        : PropertyCore(dataProvider, changed)
    {
        FATAL(dataProvider.propertyType == PropertyType::ReadOnly, "The data provider is not meant for a read-only property.");
    }

    operator Type() const
    {
        return static_cast<Type>(getDataProvider().get());
    }
};

}} // mox::metakernel

#endif // PROPERTIES_HPP
