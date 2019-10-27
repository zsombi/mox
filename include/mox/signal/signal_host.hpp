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

#ifndef SIGNAL_HOST_HPP
#define SIGNAL_HOST_HPP

#include <mox/metadata/callable.hpp>
#include <mox/utils/locks.hpp>

#include <vector>

namespace mox
{

struct AbstractSignalDescriptor;
class Signal;

/// The class is the counterpart of the Mox signals, it holds all the signals declared on a class.
/// Each class that declares signals must be derived from SignalHostNotion class.
class MOX_API SignalHostNotion
{
public:
    /// Destructor.
    virtual ~SignalHostNotion();

    /// Registers the \a signal to the signal host.
    /// \param signal The signal to register.
    /// \return The signal ID of the registered signal.
    size_t registerSignal(Signal& signal);

    /// Removes a signal from the signal host register.
    /// \param signal The signal to remove from the register.
    void removeSignal(Signal& signal);

    /// Activates the signal identified by the \a descriptor, passing the arguments packed in
    /// \a args.
    /// \return The number of activation times, or -1 if no signal was identified on the signal host.
    int activate(const AbstractSignalDescriptor& descriptor, Callable::ArgumentPack& args);

protected:
    /// Constructor.
    explicit SignalHostNotion(ObjectLock& lock);

    /// Signal host lock, guards the signal register.
    ObjectLock& m_lock;
    /// The signal register holding all declared signals on a signal host.
    std::vector<const Signal*> m_signals;
};

/// Template class adapting the SignalHostNotion over a DerivedType.
template <typename LockType>
class SignalHost : public LockType, public SignalHostNotion
{
public:
    explicit SignalHost()
        : SignalHostNotion(static_cast<LockType&>(*this))
    {
    }
};

} // namespace mox

#endif // SIGNAL_HOST_HPP
