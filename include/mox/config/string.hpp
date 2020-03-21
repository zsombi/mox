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

#ifndef STRING_HPP
#define STRING_HPP

#include <string>
#include <algorithm>

#ifdef ANDROID

#include <sstream>

namespace std
{

template<typename T>
string to_string(T const& value)
{
    stringstream stream;
    stream << value;
    return stream.str();
}

} // namespace std

#endif // ANDROID

namespace mox
{

inline std::string string_tolower(std::string data)
{
    auto predicate = [] (unsigned char c)
    {
        return tolower(c);
    };
    std::transform(data.begin(), data.end(), data.begin(), predicate);
    return data;
}

} //namespace mox

#endif // STRING_HPP
