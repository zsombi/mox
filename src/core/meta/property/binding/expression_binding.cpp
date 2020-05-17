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

#include <private/binding_p.hpp>
#include <mox/core/meta/property/binding/expression_binding.hpp>

namespace mox
{

ExpressionBinding::ExpressionBinding(ExpressionType&& expression, bool permanent)
    : Binding(permanent)
    , m_expression(std::move(expression))
{
}

void ExpressionBinding::initialize()
{
    BindingScope setCurrent(*this);
    (void)m_expression();
}

void ExpressionBinding::evaluate()
{
    if (!isEnabled() || !isAttached())
    {
        return;
    }

    auto value = m_expression();
    updateTarget(value);
}

ExpressionBindingSharedPtr ExpressionBinding::create(ExpressionType&& expression, bool permanent)
{
    auto binding = make_polymorphic_shared_ptr<Binding>(new ExpressionBinding(std::forward<ExpressionType>(expression), permanent));
    binding->initialize();
    return binding;
}

ExpressionBindingSharedPtr ExpressionBinding::bindPermanent(Property& target, ExpressionType&& expression)
{
    throwIf<ExceptionType::InvalidProperty>(!target.isValid());
    auto binding = create(std::forward<ExpressionType>(expression), true);
    binding->attach(target);
    return binding;
}

ExpressionBindingSharedPtr ExpressionBinding::bind(Property& target, ExpressionType&& expression)
{
    throwIf<ExceptionType::InvalidProperty>(!target.isValid());
    auto binding = create(std::forward<ExpressionType>(expression), false);
    binding->attach(target);
    return binding;
}

} // mox
