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

#include <mox/utils/containers/flat_map.hpp>
#include <mox/meta/base/metabase.hpp>
#include <mox/meta/core/callable.hpp>

namespace mox
{

class Signal;

/// SignalType declares the type of a signal and holds the argument signatures (descriptors)
/// of a signal type.
///
/// You must declare the signal type before using signals in Mox. You can declare signal types
/// using SignalTypeDecl<> template class.
///
/// To declare a signal which takes an \e int as argument:
/// \code static inline SignalTypeDecl<int> IntSignalType.
///
/// To declare a signal with no arguments:
/// \code static inline SignalTypeDecl<> SimpleSignalType;
///
/// To declare a signal which takes an \e int and a \s std::string as argument:
/// \code static inline SignalTypeDecl<int, std::string> IntAndStringSignalType.
///
/// \note: An object can have only one signal instance with a given argument type set. Define multiple
/// signal types with similar arguments to have multiple signals with same arguments.
class MOX_API SignalType
{
public:
    /// Destructor.
    virtual ~SignalType() = default;

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

protected:
    /// Constructor.
    SignalType(VariantDescriptorContainer&& args);

    /// Holds the argument descriptors of the signal type.
    VariantDescriptorContainer m_argumentDescriptors;

    DISABLE_COPY_OR_MOVE(SignalType)
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
