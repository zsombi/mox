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

#ifndef BINDING_NORMALIZER_HPP
#define BINDING_NORMALIZER_HPP

#include <mox/config/deftypes.hpp>
#include <mox/metadata/variant.hpp>
#include <mox/utils/locks.hpp>

namespace mox
{

class Binding;
using BindingSharedPtr = std::shared_ptr<Binding>;

class BindingNormalizer;
using BindingNormalizerPtr = std::unique_ptr<BindingNormalizer>;

/// Binding loops occur when changes on properties participating in bindings are triggering cyclic,
/// endless binding evaluation. Whereas these may be a sign of bad design of your software, there
/// may be use cases where you can use binding loops to normalize property value oscillation.
///
/// Binding loops are normalized when they are grouped. You can assign a binding normalizer to a
/// binding group by calling BindingGroup::setNormalizer() method, specifying the binding to which
/// the normalizer is targeted. When bindings are evaluated, this normalizer is called for each
/// binding participating in the binding cycle. For each binding, first the initialzie() method
/// is called, where you can set binding specific initial values. When the binding evaluation
/// reaches the same binding again, the normalize() method is called. At this point you can change
/// the value received to normalize the binding cycle, or decide to quit the binding loop silently,
/// or by throwing an exception.
///
/// BindingGroups can have only one binding loop normalizer objects at a time.
class MOX_API BindingNormalizer : public RefCountable<int>
{
    friend class BindingGroup;
    BindingSharedPtr m_target;

public:
    /// The binding loop normalization result.
    enum Result
    {
        /// The binding value is normalized, the binding can update its target property value.
        Normalized,
        /// The binding loop normalization failed, exit the binding loop silently.
        FailAndExit,
        /// The binding loop normalization failed, throw an Exception with ExceptionType::BindingLoop.
        Throw
    };
    /// Destructor.
    virtual ~BindingNormalizer() = default;

    /// Returns the target binding object of the binding loop normalizer.
    /// \return the target binding object, nullptr if none is set.
    inline BindingSharedPtr getTarget()
    {
        return m_target;
    }

    /// Initialize the binding loop normalizer. You can override this to initialize your normalizer
    /// with binding specific values.
    /// \param binding The binding to initialize your value.
    /// \param value The value evaluated on the binding at the first time.
    virtual void initialize(Binding& binding, const Variant& value)
    {
        UNUSED(binding);
        UNUSED(value);
    }

    /// Try to normalize the \a value if a \a binding at a given \a loopCount.
    /// \param binding The binding object to normalize.
    /// \param value The binding evaluation value to alter.
    /// \param loopCount The binding loop count the \a binding is visited by the binding evaluations.
    /// \return The normalization result.
    /// \sa Result.
    virtual Result tryNormalize(Binding& binding, Variant& value, size_t loopCount) = 0;

    /// Resets the binding loop normalizer object. The method is called when the binding loop
    /// normalization fails.
    virtual void reset() {}

protected:
    /// Constructor.
    explicit BindingNormalizer() = default;
};

}

#endif // BINDING_NORMALIZER_HPP
