// Copyright (C) 2020 bitWelder

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <mox/core/metakernel/property_core.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/utils/type_traits.hpp>

namespace mox { namespace metakernel {

/******************************************************************************
 * Templates
 */

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
    typedef Type ValueType;

    /// The changed signal of the property.
    metakernel::Signal<Type> changed;

    /// Constructs a property with the default property value provider.
    explicit Property(const Type& defaultValue = Type());

    /// Constructs a property with a custom property value provider.
    explicit Property(Data& dataProvider);

    ~Property();

    /// Property getter.
    operator Type() const;

    /// Property setter.
    void operator=(const Type& value);

    /// \name Bindings
    /// \{
    /// Creates a property binding between this property and an other property given as argument.
    /// The binding is a one-way binding, created with BindingPolicy::DetachOnWrite policy.
    /// \param source The source property to bind to this property.
    /// \return The binding object created.
    auto bind(Property<Type>& source, BindingPolicy policy = BindingPolicy::DetachOnWrite);
    /// \}
};

/// The PropertyBinding defines a property binding behavior.
template <class Type>
class PropertyBinding : public BindingCore
{
    Property<Type>& m_target;
    Property<Type>& m_source;

    ConnectionPtr m_sourceWatch;

public:
    static BindingPtr create(Property<Type>& target, Property<Type>& source);

protected:
    explicit PropertyBinding(Property<Type>& target, Property<Type>& source);
    ~PropertyBinding();

    void evaluateOverride() override;
    void detachOverride() override;

    void onSourceChanged(Type value);
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

/******************************************************************************
 * Implementations
 */
template <class Type>
Property<Type>::Property(const Type& defaultValue)
    : PropertyCore(*(new PropertyData(defaultValue)), changed)
{
    FATAL(getDataProvider().propertyType == PropertyType::ReadWrite, "The data provider is not meant for a writable property.");
}

template <class Type>
Property<Type>::Property(Data& dataProvider)
    : PropertyCore(dataProvider, changed)
{
    FATAL(getDataProvider().propertyType == PropertyType::ReadWrite, "The data provider is not meant for a writable property.");
}

template <class Type>
Property<Type>::~Property()
{
    auto dp = dynamic_cast<PropertyData*>(&getDataProvider());
    if (dp)
    {
        delete dp;
    }
}

template <class Type>
Property<Type>::operator Type() const
{
    notifyGet();
    return static_cast<Type>(getDataProvider().get());
}

template <class Type>
void Property<Type>::operator=(const Type& value)
{
    notifySet();
    auto& dp = getDataProvider();
    if (!dp.isEqual(value))
    {
        dp.set(value);
        changed(value);
    }
}

template <class Type>
auto Property<Type>::bind(Property<Type>& source, BindingPolicy policy)
{
    // source to target
    auto binding = PropertyBinding<Type>::create(*this, source);
    binding->setPolicy(policy);
    // Evaluate the binding.
    binding->evaluate();
    return binding;
}

/*-----------------------------------------------------------------------------
 * Bindings
 */
template <class Type>
BindingPtr PropertyBinding<Type>::create(Property<Type>& target, Property<Type>& source)
{
    auto binding = make_polymorphic_shared_ptr<BindingCore>(new PropertyBinding<Type>(target, source));
    binding->attachToTarget(target);
    return binding;
}

template <class Type>
void PropertyBinding<Type>::evaluateOverride()
{
    FATAL(m_isEnabled, "Cannot evaluate disabled bindings");
    m_target = Type(m_source);
}

template <class Type>
PropertyBinding<Type>::PropertyBinding(Property<Type>& target, Property<Type>& source)
    : m_target(target)
    , m_source(source)
{
    setEnabled(false);
    m_sourceWatch = m_source.changed.connect(*this, &PropertyBinding<Type>::onSourceChanged);
}

template <class Type>
PropertyBinding<Type>::~PropertyBinding()
{
}

template <class Type>
void PropertyBinding<Type>::detachOverride()
{
    if (m_sourceWatch && m_sourceWatch->isConnected())
    {
        m_sourceWatch->disconnect();
        m_sourceWatch.reset();
    }
}

template <class Type>
void PropertyBinding<Type>::onSourceChanged(Type value)
{
    if (isEnabled())
    {
        BindingScope currentBinding(*this);
        ScopeSignalBlocker blockSource(m_source.changed);
        m_target = value;
    }
}

/******************************************************************************
 * functions
 */
template <class... PropertyType>
BindingGroupPtr bindProperties(PropertyType&... properties)
{
    static_assert (sizeof... (properties) > 1, "at least two properties are required");
    auto group = BindingGroup::create();
    group->setPolicy(BindingPolicy::KeepOnWrite);

    std::array<std::add_pointer_t<std::remove_reference_t<get_type<0, PropertyType...>>>, sizeof...(properties)> aa = {{&properties...}};
    for (size_t i = 0; i < aa.size() - 1; ++i)
    {
        auto* target = aa[i];
        auto* source = aa[i + 1];
        group->addToGroup(*target->bind(*source));
    }
    group->addToGroup(*aa.back()->bind(*aa.front()));

    return group;
}

}} // mox::metakernel

#endif // PROPERTIES_HPP
