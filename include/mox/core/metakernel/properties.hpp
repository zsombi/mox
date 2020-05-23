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
    Type m_data;

public:
    typedef Type ValueType;

    /// The changed signal of the property.
    metakernel::Signal<Type> changed;

    /// Constructs a property with the default property value provider.
    explicit Property(const Type& defaultValue = Type());

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
    /// \param polict The binding policy to use. The default policy is BindingPolicy::DetachOnWrite.
    /// \return The binding object created.
    auto bind(Property<Type>& source, BindingPolicy policy = BindingPolicy::DetachOnWrite);

    /// Creates a binding on a property with an expression. The expression can use other properties
    /// and must return the same type as the target proprety type.
    /// \tparam ExpressionType The expression type, a function, a functor or a static member function.
    /// \param expression The expression function.
    /// \param polict The binding policy to use. The default policy is BindingPolicy::DetachOnWrite.
    /// \return The binding object created.
    template <class ExpressionType>
    auto bind(ExpressionType expression, BindingPolicy policy = BindingPolicy::DetachOnWrite);
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
};

/// Expression binding.
template <class ExpressionType>
class ExpressionBinding : public BindingCore
{
    using PropertyType = Property<typename function_traits<ExpressionType>::return_type>;
    using ConnectionContainer = std::vector<ConnectionPtr>;

    ConnectionContainer m_connections;
    PropertyType& m_target;
    ExpressionType m_expression;

public:
    static BindingPtr create(PropertyType& target, ExpressionType expression);

    void notifyPropertyAccessed(ConnectFunc connectFunc) override;

protected:
    explicit ExpressionBinding(PropertyType& target, ExpressionType expression);

    void evaluateOverride() override;
    void detachOverride() override;
};


/// This template class provides functionalities for read-only properties. Unlike the writable
/// properties, the read-only properties use value providers. These value providers can update
/// the value of the property, so the users of the properties can get notified about the change
/// of the property value. A typical use case of these types of properties is to provide status
/// updates on an entity.
template <class Type>
class StatusProperty : public PropertyCore
{
    Data& m_dataProvider;

public:
    /// The changed signal of the property.
    metakernel::Signal<Type> changed;

    explicit StatusProperty(Data& dataProvider)
        : PropertyCore(PropertyType::ReadOnly)
        , m_dataProvider(dataProvider)
    {
    }

    operator Type() const
    {
        return static_cast<Type>(m_dataProvider.get());
    }
};

/******************************************************************************
 * Implementations
 */
template <class Type>
Property<Type>::Property(const Type& defaultValue)
    : PropertyCore(PropertyType::ReadWrite)
    , m_data(defaultValue)
{
}

template <class Type>
Property<Type>::~Property()
{
}

template <class Type>
Property<Type>::operator Type() const
{
    auto currentBinding = BindingScope::getCurrent();
    if (currentBinding)
    {
        auto connectFunc = [this](BindingCore& binding)
        {
            return const_cast<Property<Type>*>(this)->changed.connect(binding, &BindingCore::evaluate);
        };
        currentBinding->notifyPropertyAccessed(connectFunc);
    }
    return m_data;
}

template <class Type>
void Property<Type>::operator=(const Type& value)
{
    notifySet();
    if (m_data != value)
    {
        m_data = value;
        changed(m_data);
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

template <class Type>
template <class ExpressionType>
auto Property<Type>::bind(ExpressionType expression, BindingPolicy policy)
{
    using ReturnType = typename function_traits<ExpressionType>::return_type;
    static_assert(std::is_same_v<ReturnType, Type>, "Expression return type must be same as the target property type.");

    auto binding = ExpressionBinding<ExpressionType>::create(*this, expression);
    binding->setPolicy(policy);
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
    if (!isEnabled())
    {
        return;
    }
    ScopeSignalBlocker blockSource(m_source.changed);
    m_target = Type(m_source);
}

template <class Type>
PropertyBinding<Type>::PropertyBinding(Property<Type>& target, Property<Type>& source)
    : m_target(target)
    , m_source(source)
{
    setEnabled(false);
    m_sourceWatch = m_source.changed.connect(*this, &PropertyBinding<Type>::evaluate);
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
    }
    m_sourceWatch.reset();
}


template <class ExpressionType>
BindingPtr ExpressionBinding<ExpressionType>::create(PropertyType& target, ExpressionType expression)
{
    auto binding = make_polymorphic_shared_ptr<BindingCore>(new ExpressionBinding<ExpressionType>(target, expression));
    binding->attachToTarget(target);
    return binding;
}

template <class ExpressionType>
ExpressionBinding<ExpressionType>::ExpressionBinding(PropertyType& target, ExpressionType expression)
    : m_target(target)
    , m_expression(expression)
{
    setEnabled(false);
}

template <class ExpressionType>
void ExpressionBinding<ExpressionType>::notifyPropertyAccessed(ConnectFunc connectFunc)
{
    m_connections.push_back(connectFunc(*this));
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
    auto disconnector = [](auto& connection)
    {
        if (connection && connection->isConnected())
        {
            connection->disconnect();
        }
    };
    for_each(m_connections, disconnector);
    m_connections.clear();
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
