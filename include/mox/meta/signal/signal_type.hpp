/*
 * Copyright (C) 2017-2019 bitWelder
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

#ifndef SIGNAL_TYPE_HPP
#define SIGNAL_TYPE_HPP

#include <mox/utils/globals.hpp>
#include <mox/utils/locks.hpp>
#include <mox/utils/containers/flat_map.hpp>
#include <mox/metatype.core/callable.hpp>

namespace mox
{

class Signal;

/// SignalType declares the type of the signal. Holds the argument signatures (descriptors)
/// of a signal type, and the instances of the signal.
///
/// You must declare the signal type before using signals in Mox. To declare the signal type
/// put a static member in your class using SignalTypeDecl<> template.
///
/// To declare a signal which takes an \e int as argument:
/// \code static inline SignalTypeDecl<int> IntSignalType.
///
/// To declare a signal with no arguments:
/// \code static inline SignalTypeDecl<> SimpleSignalType;
class MOX_API SignalType : public ObjectLock
{
public:
    /// Destructor.
    ~SignalType() = default;

    Signal* getSignalForInstance(ObjectLock& instance) const;

    /// Activates the signal on an instance.
    /// \param sender The signal sender instance.
    /// \param args The arguments to pass to the signal packed for metainvocation.
    /// \return The activation count, the number of times the signal was activated.
    /// If the signal is not activable with the arguments, or the sender has no
    /// signal with this type, returns -1.
    int activate(ObjectLock& sender, const Callable::ArgumentPack& args = Callable::ArgumentPack()) const;

    template <class SenderObject, typename... Arguments>
    int emit(SenderObject& sender, Arguments... args) const
    {
        if (!m_argumentDescriptors.isInvocableWith(VariantDescriptorContainer::getArgs<Arguments...>()))
        {
            return -1;
        }
        return activate(sender, Callable::ArgumentPack(args...));
    }

    /// Checks if a signal type is compatible with the \a other. Two signal types
    /// are compatible if their arguments are compatible. Two argument sets are
    /// compatible if the caller (\a other) signal arguments are the same amount
    /// or more than the callee (this) argument count, and the arguments are convertible
    /// between each other.
    /// \param other The other signal type to test.
    /// return If this signal is callable by the \a other, returns \e true, otherwise \e false.
    bool isCompatible(const SignalType& other) const;

    /// Returns the argument descriptors of the signal type.
    /// \return The argument descriptors of the signal type.
    const VariantDescriptorContainer& getArguments() const;

    /// Adds a signal instance to the signal type.
    /// \param signal The signal instance to add.
    void addSignalInstance(Signal& signal);

    /// Removes a signal instance from the signal type.
    /// \param signal The signal instance to remove.
    void removeSignalInstance(Signal& signal);

protected:
    /// Constructor.
    SignalType(VariantDescriptorContainer&& args);

    using InstanceContainer = FlatMap<ObjectLock*, Signal*>;
    /// The instances of the signal type.
    InstanceContainer m_instances;
    /// Holds the argument descriptors of the signal type.
    VariantDescriptorContainer m_argumentDescriptors;
};

/// Signal type declarator template. Use this template to declare your signal types
/// in your class.
template <typename... Arguments>
class SignalTypeDecl : public SignalType
{
public:
    /// Constructor.
    SignalTypeDecl()
        : SignalType(VariantDescriptorContainer::ensure<Arguments...>())
    {
    }
};

} // namespace mox

#endif // SIGNAL_TYPE_HPP
