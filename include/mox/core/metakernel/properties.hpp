// Copyright (C) 2020 bitWelder

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <mox/core/metakernel/property_core.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/utils/type_traits.hpp>
#include <mox/utils/log/logger.hpp>

namespace mox { namespace metakernel {

/******************************************************************************
 * Templates
 */

/// This template class provides functionalities for read-only properties. Unlike the writable
/// properties, the read-only properties use value providers. These value providers can update
/// the value of the property, so the users of the properties can get notified about the change
/// of the property value. A typical use case of these types of properties is to provide status
/// updates on an entity.
///
/// You cannot bind properties to a status property, but you can use the status property in
/// bindings as source, or as part of an expression.
template <class Type>
class StatusProperty : public StatusPropertyCore
{
    using Self = StatusProperty<Type>;

public:
    using ChangedSignal = metakernel::Signal<Type>;

    /// Status property data provider type. Provides the data of the status property and an
    /// interface to update the property.
    class Data
    {
        Self* m_property = nullptr;

    public:
        /// Destructor.
        virtual ~Data() = default;
        /// Attaches a property data provider to a \a property.
        /// \param property The property the data provider is to attach.
        void attach(Self& property);
        /// Updates the property the data provider is attached to.
        void update();

        /// Property data getter.
        virtual Type get() const = 0;
    };
    /// The changed signal of the property.
    ChangedSignal changed;

    /// Construct a status property instance using a \a dataProvider.
    explicit StatusProperty(Lockable& host, Data& dataProvider);

    /// Property getter.
    /// \return The value of the property.
    operator Type() const;

private:
    Data& m_dataProvider;
};

/// The template class defines a read-write property with a change signal. Provides a default
/// property data provider.
template <class Type>
class Property : public PropertyCore
{
public:
    using ValueType = Type;
    using ChangedSignal = metakernel::Signal<Type>;

    /// The changed signal of the property.
    ChangedSignal changed;

    /// Constructs a property with the default property value provider.
    explicit Property(Lockable& host, const Type& defaultValue = Type());

    /// Property getter.
    operator Type() const;

    /// Property setter.
    void operator=(const Type& value);

    /// \name Bindings
    /// \{

    /// Creates a binding between two properties. The method accepts writable properties aswell
    /// as status properties.
    /// \tparam BindingSource The type of the binding source.
    /// \param source The binding source.
    /// \param polict The binding policy to use. The default policy is BindingPolicy::DetachOnWrite.
    /// \return The binding object created.
    template <class BindingSource>
    std::enable_if_t<std::is_base_of_v<PropertyCore, BindingSource> || std::is_base_of_v<StatusProperty<Type>, BindingSource>, BindingPtr>
    bind(BindingSource& source, BindingPolicy policy = BindingPolicy::DetachOnWrite);

    /// Creates a binding on a property and an expression as source.
    /// \tparam ExpressionType The type of the binding source.
    /// \param source The binding expression source.
    /// \param polict The binding policy to use. The default policy is BindingPolicy::DetachOnWrite.
    /// \return The binding object created. The binding takes the ownership over the expression.
    template <class ExpressionType>
    std::enable_if_t<!std::is_base_of_v<PropertyCore, ExpressionType> && !std::is_base_of_v<StatusProperty<Type>, ExpressionType>, BindingPtr>
    bind(ExpressionType source, BindingPolicy policy = BindingPolicy::DetachOnWrite);
    /// \}

private:
    Type m_data;
};

/// Template class implementing bindings to a property and an other property type. The source
/// property type is either a writable property or a status property.
template <class Type, class PropertyType>
class PropertyTypeBinding : public BindingCore, public SlotHolder
{
    using Self = PropertyTypeBinding<Type, PropertyType>;

    Property<Type>& m_target;
    PropertyType& m_source;

public:
    /// Creates a binding object between a target property and a source.
    static BindingPtr create(Property<Type>& target, PropertyType& source);

protected:
    /// Constructor.
    explicit PropertyTypeBinding(Property<Type>& target, PropertyType& source);
    /// Overrides BindingCore::evaluateOverride().
    void evaluateOverride() override;
    /// Overrides BindingCore::detachOverride().
    void detachOverride() override;
};

/// Template class implementing bindings to a property and an expression. The expression is
/// a function or a lambda that can use other properties. An example is a binding that converts
/// a property from one type into an other.
template <class ExpressionType>
class ExpressionBinding : public BindingCore, public SlotHolder
{
    using PropertyType = Property<typename function_traits<ExpressionType>::return_type>;

    PropertyType& m_target;
    ExpressionType m_expression;

public:
    /// Creates a binding to \a target, using an \a expression.
    static BindingPtr create(PropertyType& target, ExpressionType expression);

protected:
    /// Constructor.
    explicit ExpressionBinding(PropertyType& target, ExpressionType expression);
    /// Overrides BindingCore::evaluateOverride().
    void evaluateOverride() override;
    /// Overrides BindingCore::detachOverride().
    void detachOverride() override;
};


/******************************************************************************
 * Implementations
 */
template <class Type>
void StatusProperty<Type>::Data::attach(Self& property)
{
    m_property = &property;
}

template <class Type>
void StatusProperty<Type>::Data::update()
{
    m_property->changed(get());
}

template <class Type>
StatusProperty<Type>::StatusProperty(Lockable& host, Data& dataProvider)
    : StatusPropertyCore(host)
    , changed(host)
    , m_dataProvider(dataProvider)
{
    m_dataProvider.attach(*this);
}

template <class Type>
StatusProperty<Type>::operator Type() const
{
    notifyGet(const_cast<ChangedSignal&>(changed));
    return m_dataProvider.get();
}


template <class Type>
Property<Type>::Property(Lockable& host, const Type& defaultValue)
    : PropertyCore(host)
    , changed(host)
    , m_data(defaultValue)
{
}

template <class Type>
Property<Type>::operator Type() const
{
    notifyGet(const_cast<ChangedSignal&>(changed));
    return m_data;
}

template <class Type>
void Property<Type>::operator=(const Type& value)
{
    notifySet();
    lock_guard lock(*this);
    if (m_data != value)
    {
        m_data = value;
        ScopeRelock re(*this);
        changed(m_data);
    }
}

template <class Type>
template <class BindingSource>
std::enable_if_t<std::is_base_of_v<PropertyCore, BindingSource> || std::is_base_of_v<StatusProperty<Type>, BindingSource>, BindingPtr>
Property<Type>::bind(BindingSource& source, BindingPolicy policy)
{
    auto binding = PropertyTypeBinding<Type, BindingSource>::create(*this, source);
    binding->attachToTarget(*this);
    binding->setPolicy(policy);
    binding->evaluate();
    return binding;
}

template <class Type>
template <class ExpressionType>
std::enable_if_t<!std::is_base_of_v<PropertyCore, ExpressionType> && !std::is_base_of_v<StatusProperty<Type>, ExpressionType>, BindingPtr>
Property<Type>::bind(ExpressionType source, BindingPolicy policy)
{
    using ReturnType = typename function_traits<ExpressionType>::return_type;
    static_assert(std::is_same_v<ReturnType, Type>, "Expression return type must be same as the target property type.");
    auto binding = ExpressionBinding<ExpressionType>::create(*this, source);
    binding->attachToTarget(*this);
    binding->setPolicy(policy);
    binding->evaluate();
    return binding;
}


/*-----------------------------------------------------------------------------
 * Bindings
 */
template <class Type, class PropertyType>
BindingPtr PropertyTypeBinding<Type, PropertyType>::create(Property<Type>& target, PropertyType& source)
{
    return make_polymorphic_shared_ptr<BindingCore>(new Self(target, source));
}

template <class Type, class PropertyType>
PropertyTypeBinding<Type, PropertyType>::PropertyTypeBinding(Property<Type>& target, PropertyType& source)
    : m_target(target)
    , m_source(source)
{
    setEnabled(false);
}

template <class Type, class PropertyType>
void PropertyTypeBinding<Type, PropertyType>::evaluateOverride()
{
    if (!isEnabled())
    {
        return;
    }
    detachOverride();
    m_target = Type(m_source);
}

template <class Type, class PropertyType>
void PropertyTypeBinding<Type, PropertyType>::detachOverride()
{
    disconnectAll();
}


template <class ExpressionType>
BindingPtr ExpressionBinding<ExpressionType>::create(PropertyType& target, ExpressionType expression)
{
    return make_polymorphic_shared_ptr<BindingCore>(new ExpressionBinding<ExpressionType>(target, expression));
}

template <class ExpressionType>
ExpressionBinding<ExpressionType>::ExpressionBinding(PropertyType& target, ExpressionType expression)
    : m_target(target)
    , m_expression(expression)
{
    setEnabled(false);
}

template <class ExpressionType>
void ExpressionBinding<ExpressionType>::evaluateOverride()
{
    if (!isEnabled())
    {
        return;
    }
    // clear the connections, we must rebuild them each time the expression is evaluated
    detachOverride();
    m_target = m_expression();
}

template <class ExpressionType>
void ExpressionBinding<ExpressionType>::detachOverride()
{
    disconnectAll();
}

/******************************************************************************
 * functions
 */
template <class... PropertyType>
BindingGroupPtr bindProperties(PropertyType&... properties)
{
    static_assert (sizeof... (properties) > 1, "at least two properties are required");
    auto group = BindingGroup::create();
    auto policy = BindingPolicy::KeepOnWrite;
    group->setPolicy(policy);

    std::array<std::add_pointer_t<std::remove_reference_t<get_type<0, PropertyType...>>>, sizeof...(properties)> aa = {{&properties...}};
    for (size_t i = 0; i < aa.size() - 1; ++i)
    {
        auto* target = aa[i];
        auto* source = aa[i + 1];
        group->addToGroup(*target->bind(*source, policy));
    }
    group->addToGroup(*aa.back()->bind(*aa.front(), policy));

    return group;
}

}} // mox::metakernel

DECLARE_LOG_CATEGORY(bindings)

#endif // PROPERTIES_HPP
