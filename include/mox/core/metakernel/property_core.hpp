// Copyright (C) 2020 bitWelder

#ifndef PROPERTY_CORE_HPP
#define PROPERTY_CORE_HPP

#include <mox/config/pimpl.hpp>
#include <mox/config/platform_config.hpp>
#include <mox/core/metakernel/argument_data.hpp>
//#include <mox/utils/locks.hpp>
#include <mox/utils/containers/shared_vector.hpp>

namespace mox { namespace metakernel {

// Forward declarations
class SignalCore;
class PropertyCore;
class BindingCore;
using BindingPtr = std::shared_ptr<BindingCore>;
using BindingWeakPtr = std::weak_ptr<BindingCore>;
class BindingGroup;
using BindingGroupPtr = std::shared_ptr<BindingGroup>;


/// The property type.
enum class PropertyType
{
    ReadOnly,
    ReadWrite
};

/// The BindingCore class provides core functionality of the bindings on properties.
class MOX_API BindingCore : public std::enable_shared_from_this<BindingCore>
{
public:
    /// Destructor.
    virtual ~BindingCore();

    bool isEnabled() const;
    void setEnabled(bool enabled);

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

    virtual void detachOverride() {}
    virtual void setEnabledOverride() {}

    enum class Status : byte
    {
        Detaching,
        Detached,
        Attaching,
        Attached
    };

    PropertyCore* m_target = nullptr;
    BindingGroupPtr m_group;
    Status m_status = Status::Detached;
    bool m_isEnabled = true;
};

/// The PropertyCore class provides the core functionality of the properties. Holds the
/// attributes and the bindings of a property.
class PropertyCorePrivate;
class MOX_API PropertyCore
{
    friend class BindingCore;

public:
    /// Property data interface. Provides generic and non-thread safe access to the property
    /// data and type. Derive your property data providers from this class when creating
    /// properties with custom data provider.
    class MOX_API Data
    {
    public:
        /// The property type.
        const PropertyType propertyType;

        /// Constructs a property data provider with the property type specified.
        /// \param type The property type.
        explicit Data(PropertyType type);
        /// Default destructor.
        virtual ~Data() = default;

        /// Property getter, returns the property value in an opaque argument data.
        /// \return The argument value enclosed in an opaque argument data.
        virtual ArgumentData get() const = 0;
        /// Property setter, sets the value of the property using an opaque argument
        /// data. Implementations are not expected to activate the property changed
        /// signal.
        /// \param data The property data to set.
        virtual void set(const ArgumentData& data) = 0;

        virtual bool isEqual(const ArgumentData& other) = 0;
    };

    /// Destructor.
    ~PropertyCore();

    /// Gets the type of the property.
    /// \return The type of the property.
    PropertyType getType() const;

protected:
    /// Constructs a property core using a proeprty data provider.
    PropertyCore(Data& data, SignalCore& changedSignal);

    /// Returns the data provider of a property.
    /// \return The data provider of a property.
    Data& getDataProvider();
    Data& getDataProvider() const;

    void addBinding(BindingCore& binding);
    void removeBinding(BindingCore& binding);

    ArgumentData get() const;
    void set(const ArgumentData& data);

    struct ZeroBindingCheck
    {
        bool operator()(BindingPtr binding)
        {
            return !binding || !binding->isAttached();
        }
    };
    struct ZeroBindingSet
    {
        void operator()(BindingPtr& binding)
        {
            if (binding && binding->isAttached())
            {
                binding->detachFromTarget();
            }
            binding.reset();
        }
    };
    using BindingsStorage = SharedVector<BindingPtr, ZeroBindingCheck, ZeroBindingSet>;

    BindingsStorage m_bindings;
    PropertyCore::Data& m_data;
    SignalCore& m_changedSignal;
};

/// The BindingGroup is a binding type which groups individual bindings to act as one.
/// The first binding attached to the group serves as the main binding that evaluates
/// and holds the target of the binding.
class MOX_API BindingGroup : public BindingCore
{
public:
    explicit BindingGroup() = default;
    ~BindingGroup() override;

    template <class... BindingPtrType>
    explicit BindingGroup(BindingPtrType... bindings)
    {
        auto tupleArg = std::tuple<BindingPtrType...>(bindings...);
        std::apply([this](auto ...x){ (static_cast<void>(addToGroup(*x)), ...);}, tupleArg);
    }

    void addToGroup(BindingCore& binding);
    void removeFromGroup(BindingCore& binding);

protected:
    void setEnabledOverride() override;

private:
    std::vector<BindingPtr> m_bindings;
};

}} // mox::metakernel

#endif // PROPERTY_CORE_HPP
