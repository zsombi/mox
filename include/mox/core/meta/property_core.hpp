// Copyright (C) 2020 bitWelder

#ifndef PROPERTY_CORE_HPP
#define PROPERTY_CORE_HPP

#include <mox/config/pimpl.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/meta/argument_data.hpp>
#include <mox/core/meta/lockable.hpp>
#include <mox/core/meta/signals.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <functional>

namespace mox
{

// Forward declarations
class SignalCore;
class PropertyCore;
class BindingCore;
using BindingPtr = std::shared_ptr<BindingCore>;
using BindingWeakPtr = std::weak_ptr<BindingCore>;
class BindingGroup;
using BindingGroupPtr = std::shared_ptr<BindingGroup>;
using BindingGroupWeakPtr = std::weak_ptr<BindingGroup>;


/// The BindingPolicy enum class defines the policy of a binding used when the user sets the
/// value of a property manually.
enum class BindingPolicy
{
    /// The binding is detached when you write on the target property, except if the write occurs
    /// due to a binding that is the current one. In this case the policy is ignored.
    DetachOnWrite,
    /// The binding is kept on the property no matter what is teh cause of the property setter
    /// call.
    KeepOnWrite
};

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

    /// Signal connection template function type, expecting a binding and returning a signal
    /// connection token.
    using ConnectFunc = std::function<ConnectionPtr(BindingCore&)>;

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

/// The StatusPropertyCore class provides the core functionality for the status properties.
/// Mox status properties are read-only properties.
class MOX_API StatusPropertyCore : public Lockable
{
public:
    /// Destructor.
    ~StatusPropertyCore() = default;

protected:
    /// Constructor.
    StatusPropertyCore(Lockable& host);

    /// Called by the property getter to notify the active binding that the property getter
    /// is called. The binding connects to the changed signal passed as argument to receive
    /// notification about the property value changes.
    /// \param changedSignal The changed signal of the property the binding connects.
    void notifyGet(SignalCore& changedSignal) const;
};

/// The PropertyCore class provides the core functionality of the properties. Holds the
/// attributes and the bindings of a property.
class PropertyCorePrivate;
class MOX_API PropertyCore : public StatusPropertyCore
{
public:
    /// Destructor.
    ~PropertyCore();

protected:
    /// Constructs a property core using a proeprty data provider.
    PropertyCore(Lockable& host);

    /// Called by the property setter, notifies the property to remove the bindings which have
    /// BindingPolicy::DetachOnWrite policy set.
    void notifySet();

private:
    DECLARE_PRIVATE_PTR(PropertyCore);
};

/// The BindingGroup class groups a individual bindings to act as one. The bindings added to a
/// group have the same policy.
///
/// It is recommended to set the group policy before adding bindings to it.
class MOX_API BindingGroup : public std::enable_shared_from_this<BindingGroup>
{
public:
    static BindingGroupPtr create();
    ~BindingGroup();

    /// Discards the group by removing all its bindings. As result of this call, the group is
    /// orphaned, and potentially destroyed.
    void discard();
    /// Adds a binding to a group, and sets the group holding the binding.
    BindingGroup& addToGroup(BindingCore& binding);
    /// Removes a binding from a group, and resets the group of the binding.
    void removeFromGroup(BindingCore& binding);

    /// Sets the policy of the group.
    /// \param policy The policy to set. All the bindings grouped are updated with the policy
    /// set.
    void setPolicy(BindingPolicy policy);

    /// Returns the enabled state of a binding group.
    /// \return The enabled state of the binding group.
    bool isEnabled() const;
    /// Sets the enabled state of a binding group.
    /// \param enabled The enabled state to set. All the bindings grouped are updated with the
    /// state set.
    void setEnabled(bool enabled);

protected:
    /// Constructor.
    explicit BindingGroup();

private:
    SharedVector<BindingPtr> m_bindings;
    std::atomic<BindingPolicy> m_policy = BindingPolicy::KeepOnWrite;
    std::atomic_bool m_isEnabled = true;
    std::atomic_bool m_isUpdating = false;
};

} // mox

#endif // PROPERTY_CORE_HPP
