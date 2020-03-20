/*
 * Copyright (C) 2017-2020 bitWelder
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>
 */

#ifndef METABASE_HPP
#define METABASE_HPP

#include <mox/utils/locks.hpp>
#include <mox/config/pimpl.hpp>
#include <mox/meta/core/variant.hpp>
#include <mox/meta/core/callable.hpp>

#include <map>
#include <memory>

namespace mox
{

class Property;
class PropertyType;
class Signal;
class SignalType;

class MetaBasePrivate;
/// MetaBase is the base class for classes that need support for properties and signals. Deriving from MetaBase
/// allows you to add properties and signal to your class either statically, or dynamically. You add properties
/// statically using the Property class, and signals using Signal class.
///
/// To add properties dynamically to your class call setProperty(). These dynamic properties exist for the entire
/// lifetime of the object, and are deleted together with the object. In case you hold a pointer with the dynamic
/// property created, the property is invalidated when the host object is deleted.
///
/// One additional feature the class provides is thread-locking. If you need a functionality that requires locking
/// the entire object, you can derive your class from SharedLock<> and pass the MetaBase instance to which your
/// objects are connected. Doing that enables your objects to be seen as mutexes. Properties and signals use the
/// same approach.
class MOX_API MetaBase : public AtomicRefCounted<int32_t>
{
    DECLARE_PRIVATE_PTR(MetaBase)

    mutable std::mutex m_mutex;

#ifdef DEBUG
    mutable atomic_int32_t m_lockCount = 0;
    mutable std::atomic<std::thread::id> m_owner = std::thread::id();
#endif

public:
    /// Construct the object lock.
    explicit MetaBase();
    /// Destruct the object lock. The destruction fails if the object's lock is still shared.
    virtual ~MetaBase();

    /// Locks the object.
    void lock();

    /// Unlocks the object.
    void unlock();

    /// Tries to lock the object.
    /// \return If the object locking succeeds, returns \e true, otherwise \e false.
    bool try_lock();

    /// Find a signal with a given type on the object.
    Signal* findSignal(const SignalType& type) const;
    /// Adds a signal to the object.
    Signal* addSignal(const SignalType& type);
    /// Activates a signal with a given type defined on this object.
    int activateSignal(const SignalType& type, const Callable::ArgumentPack& args = Callable::ArgumentPack());

    /// Finds a property with a \a type on the object. The property can be static or dynamic.
    Property* findProperty(const PropertyType& type) const;
    /// Set the property \a value on the object identified by its \a type. If the property does not
    /// exist, creates a dynamic property on the object.
    Property* setProperty(const PropertyType& type, const Variant& value);
    /// Returns the value of a property defined on the object identified by a \a type.
    Variant getProperty(const PropertyType& type) const;

    // Templates
    template <typename T>
    Property* setProperty(const PropertyType& type, T const& value)
    {
        return setProperty(type, Variant(value));
    }
};

} // mox

#endif // METABASE_HPP
