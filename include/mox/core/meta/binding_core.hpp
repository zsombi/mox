#ifndef BINDING_CORE_HPP
#define BINDING_CORE_HPP

#include <mox/config/platform_config.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/core/meta/binding_policy.hpp>

namespace mox
{

class PropertyCore;

/// The BindingCore class provides core functionality of the bindings on properties.
class BindingCorePrivate;
class MOX_API BindingCore : public std::enable_shared_from_this<BindingCore>
{
    DECLARE_PRIVATE_PTR(BindingCore);

public:
    /// Destructor.
    virtual ~BindingCore();

    /// Evaluates the binding.
    void evaluate();

    /// Returns the enabled state of a binding.
    /// \return If the binding is enabled, returns \e true, otherwise \e false.
    bool isEnabled() const;
    /// Sets the enabled state of a binding.
    /// \param enabled The enabled state of the binding.
    void setEnabled(bool enabled);

    /// Returns the binding policy of a binding.
    /// \return The binding policy of a binding. The default policy value is BindingPolicy::DetachOnWrite.
    BindingPolicy getPolicy() const;
    /// Sets the binding policy of a binding.
    /// \param policy The policy of the binding to set.
    void setPolicy(BindingPolicy policy);

    /// Attaches a binding to a property. This property stands as the target of the binding.
    /// \param property The target proeprty to attach.
    /// \throws ExceptionType::BindingAlreadyAttached if the property is already attached.
    void attachToTarget(PropertyCore& property);
    /// Detaches a binding from its target.
    /// \throws ExceptionType::BindingNotAttached if the property is already detached.
    void detachFromTarget();
    /// Returns the attached state of the binding.
    /// \return If the binding is attached to a target property, returns \e true, otherwise
    /// \e false;
    bool isAttached() const;

    /// Sets the group of a binding.
    /// \param group The binding group to set, \e nullptr to reset the group the binding belongs.
    void setGroup(BindingGroupPtr group);

protected:
    /// Constructor.
    BindingCore();

    /// \name Overridables
    /// These overridables allow you to execute custom actions during certain phases of the binding.
    /// \{
    /// The method is called when a binding is evaluated.
    virtual void evaluateOverride() {}
    /// The method is called when a binding is attaching to a target property.
    virtual void attachOverride() {}
    /// The method is called when a binding is detaching from the attached target property.
    virtual void detachOverride() {}
    /// The method is called when the binding is enabled or disabled.
    virtual void setEnabledOverride() {}
    /// The method is called when the policy of a binding is changed.
    virtual void setPolicyOverride() {}
    /// \}
};

/// Helper class, scopes the active binding evaluated.
class MOX_API BindingScope
{
    BindingWeakPtr m_previousBinding;

public:
    /// Scope constructor, takes a scope object to set as the active binding.
    explicit BindingScope(BindingCore& currentBinding);
    /// Scope destructor, restores teh previous binding as active.
    ~BindingScope();

    /// Returns the current active binding.
    /// \return The binding that is the current active one.
    static BindingPtr getCurrent();
};

} // mox

#endif // BINDING_CORE_HPP
