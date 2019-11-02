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
#include <mox/signal/signal.hpp>

#include <vector>

namespace mox
{

/// The class is the counterpart of the Mox signals, it holds all the signals declared on a class.
/// Each class that declares signals must be derived from SignalHostConcept class.
class MOX_API SignalHostConcept
{
public:
    /// Destructor.
    virtual ~SignalHostConcept();

    /// Registers the \a signal to the signal host.
    /// \param signal The signal to register.
    /// \return The signal ID of the registered signal.
    size_t registerSignal(Signal::SignalId& signal);

    /// Removes a signal from the signal host register.
    /// \param signal The signal to remove from the register.
    void removeSignal(Signal::SignalId& signal);

protected:
    /// Constructor.
    explicit SignalHostConcept();

    /// The signal register holding all declared signals on a signal host.
    std::vector<const Signal::SignalId*> m_signals;
};

/// Template class adapting the SignalHostConcept over a DerivedType.
template <typename LockType>
class SignalHost : public LockType, public SignalHostConcept
{
    LockType& getSelf()
    {
        return static_cast<LockType&>(*this);
    }

public:
    /// Activates the signal identified by the \a descriptor, passing the arguments packed in \a args.
    /// \return The number of activation times, or -1 if no signal was identified on the signal host.
    int activate(const SignalDescriptorBase& descriptor, Callable::ArgumentPack& args);
};


template <typename LockType>
int SignalHost<LockType>::activate(const SignalDescriptorBase& descriptor, Callable::ArgumentPack& args)
{
    LockType& self = getSelf();
    lock_guard{self};
    for (auto& signal : m_signals)
    {
        if (signal->isA(descriptor))
        {
            ScopeUnlock unlock(self);
            return signal->getSignal().activate(args);
        }
    }

    return -1;
}

} // namespace mox

#endif // SIGNAL_HOST_HPP
