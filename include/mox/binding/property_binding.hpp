#ifndef PROPERTY_BINDING_HPP
#define PROPERTY_BINDING_HPP

#include <mox/binding/binding.hpp>

namespace mox
{

class PropertyBinding;
using PropertyBindingSharedPtr = std::shared_ptr<PropertyBinding>;

class PropertyBindingPrivate;
/// Property binding. The property binding is realized between two properties. The binding created is a one-way
/// binding. The binding is evaluated when the source property is changed. Use this to bind two properties to
/// each other.
///
/// Use BindingGroup to enable two-way binding.
class MOX_API PropertyBinding : public Binding
{
    DECLARE_PRIVATE(PropertyBinding)

public:
    /// Creates a property binding with a source. You must attach the binding to a target to evaluate.
    /// \param source The property binding source property, which provides the value for the parget.
    /// \param permanent If the binding is meant to survive write operation on target property, set to \e true.
    /// If the binding is meant to discard on target property write, set to \e false.
    /// \return The property binding created, in detached state.
    static PropertyBindingSharedPtr create(Property& source, bool permanent);

    /// Creates a permanent binding between a \a target and \a source property, and attaches the binding to
    /// the \a target. The binding survives property writes on target.
    /// \param target The target property.
    /// \param source The source property.
    /// \return The binding handler, nullptr if \a target or both \a target and \a source are read-only.
    static PropertyBindingSharedPtr bindPermanent(Property& target, Property& source);

    /// Creates an auto-discarding binding between a \a target and \a source property, and attaches the binding
    /// to the \a target. The binding is discarded when write operation occurs on \a target.
    /// \param target The target property.
    /// \param source The source property.
    /// \return The binding handler, nullptr if \a target or both \a target and \a source are read-only. The
    /// binding is added to the target.
    static PropertyBindingSharedPtr bindAutoDiscard(Property& target, Property& source);

protected:
    /// Constructor.
    explicit PropertyBinding(Property& source, bool isPermanent);

    /// Evaluates property binding.
    void evaluate() override;
};

} // mox

#endif // PROPERTY_BINDING_HPP
