#ifndef BINDING_POLICY_HPP
#define BINDING_POLICY_HPP

namespace mox
{

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

class BindingCore;
using BindingPtr = std::shared_ptr<BindingCore>;
using BindingWeakPtr = std::weak_ptr<BindingCore>;

class BindingGroup;
using BindingGroupPtr = std::shared_ptr<BindingGroup>;
using BindingGroupWeakPtr = std::weak_ptr<BindingGroup>;

} // mox

#endif // BINDING_POLICY_HPP
