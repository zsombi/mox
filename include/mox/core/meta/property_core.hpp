// Copyright (C) 2020 bitWelder

#ifndef PROPERTY_CORE_HPP
#define PROPERTY_CORE_HPP

#include <mox/config/pimpl.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/meta/argument_data.hpp>
#include <mox/core/meta/binding_policy.hpp>
#include <mox/core/meta/lockable.hpp>
#include <mox/core/meta/signals.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <functional>

namespace mox
{

// Forward declarations
class SignalCore;
class PropertyCore;

/// The StatusPropertyCore class provides the core functionality for the status properties.
/// Mox status properties are read-only properties.
class MOX_API StatusPropertyCore : public SharedLock<Lockable>
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
