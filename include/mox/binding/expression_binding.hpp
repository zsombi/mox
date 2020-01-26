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

#ifndef EXPRESSION_BINDING_HPP
#define EXPRESSION_BINDING_HPP

#include <mox/binding/binding.hpp>

namespace mox
{

class ExpressionBinding;
using ExpressionBindingSharedPtr = std::shared_ptr<ExpressionBinding>;

/// ExpressionBinding class provides bindings with expressions. Expressions are functions with no argument
/// that return a variant. The expressions typically contain some arythmetics or other type of logic with
/// other properties involved. The expression evaluates automatically whenever a property that is participating
/// in the expression is changed.
class MOX_API ExpressionBinding : public Binding
{
public:
    /// The expression functor type.
    using ExpressionType = std::function<Variant()>;

    /// Creates an expression binding with the expression. The binding is detached.
    /// \param expression The binding expression.
    /// \param permanent If the binding is permanent, pass \e true. If the binding is discardable,
    /// pass \e false.
    /// \return The expression binding object ready to attach.
    static ExpressionBindingSharedPtr create(ExpressionType&& expression, bool permanent);

    /// Creates a permanent expression binding and attaches it to the \a target property.
    /// \param target The target property.
    /// \param expression The binding expression.
    /// \return The expression binding attached to the \a target.
    static ExpressionBindingSharedPtr bindPermanent(Property& target, ExpressionType&& expression);

    /// Creates a detachable expression binding and attaches it to the \a target property.
    /// \param target The target property.
    /// \param expression The binding expression.
    /// \return The expression binding attached to the \a target.
    static ExpressionBindingSharedPtr bindAutoDetach(Property& target, ExpressionType&& expression);

protected:
    /// Constructor.
    explicit ExpressionBinding(ExpressionType&& expression, bool permanent);

    void initialize();

    /// Override of Binding::evaluate().
    void evaluate() override;

private:
    ExpressionType m_expression;
};

} // mox

#endif // EXPRESSION_BINDING_HPP
