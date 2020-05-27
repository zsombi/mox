// Copyright (C) 2020 bitWelder

#ifndef PROPERTY_CORE_HPP
#define PROPERTY_CORE_HPP

#include <mox/config/pimpl.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/metakernel/argument_data.hpp>
#include <mox/core/metakernel/signals.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <functional>

namespace mox { namespace metakernel {

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

    /// The method is called by the target property getter to notify the current binding that
    /// the getter of the property is called. The proeprty passes on a function template, which
    /// the binding uses to create the connection to the property change signal, to get notified
    /// when the property is changed.
    /// This method is overridden by the expression bindings to gather the properties participating
    /// in the expression.
    virtual void notifyPropertyAccessed(ConnectFunc) {}

protected:
    /// Constructor.
    BindingCore();

    virtual void evaluateOverride() {}
    virtual void detachOverride() {}
    virtual void setEnabledOverride() {}
    virtual void setPolicyOverride() {}
};

/// Helper class, scopes the active binding executed.
class MOX_API BindingScope
{
    BindingWeakPtr m_previousBinding;
public:
    explicit BindingScope(BindingCore& currentBinding);
    ~BindingScope();

    static BindingPtr getCurrent();
};

/// The PropertyCore class provides the core functionality of the properties. Holds the
/// attributes and the bindings of a property.
class PropertyCorePrivate;
class MOX_API PropertyCore
{
public:
    /// Destructor.
    ~PropertyCore();

protected:
    /// Constructs a property core using a proeprty data provider.
    PropertyCore();

    using PropertyChangeConnector = std::function<ConnectionPtr(BindingCore&)>;
    void notifyGet(PropertyChangeConnector connectChange) const;
    /// Cleans the bindings.
    void notifySet();

private:
    DECLARE_PRIVATE_PTR(PropertyCore);
};

/// The BindingGroup is a binding type which groups individual bindings to act as one.
/// The first binding attached to the group serves as the main binding that evaluates
/// and holds the target of the binding.
///
/// The bindings added to a group have the same policy. It is recommended to set the group
/// policy before adding bindings to it.
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

}} // mox::metakernel

#endif // PROPERTY_CORE_HPP
