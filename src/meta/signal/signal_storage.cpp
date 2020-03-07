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

#include <signal_p.hpp>
#include <metabase_p.hpp>

namespace mox
{

SignalStorage::SignalStorage(Signal& signal, MetaBase& host, const SignalType& type)
    : host(host)
    , type(type)
    , p_ptr(&signal)
{
    MetaBasePrivate::get(host)->addSignal(*this);
}

SignalStorage::~SignalStorage()
{
}

void SignalStorage::destroy()
{
    for_each(connections, [](Signal::ConnectionSharedPtr connection) { if (connection) connection->m_signal = nullptr; });
    MetaBasePrivate::get(host)->removeSignal(this);
    p_ptr->d_ptr.reset();
}

} // mox
