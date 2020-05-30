#ifndef BINDINGS_HPP
#define BINDINGS_HPP

#include <mox/core/meta/binding_core.hpp>
#include <mox/core/meta/binding_policy.hpp>

namespace mox
{

// Forward declarations.
template <class _T>
class Property;

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

} // mox

#endif // BINDINGS_HPP
