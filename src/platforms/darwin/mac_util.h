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

#ifndef MAC_UTIL_H
#define MAC_UTIL_H

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
@class RunLoopModeTracker;
#else
typedef struct objc_object RunLoopModeTracker;
#endif

namespace mox
{
namespace mac
{

template <typename T, typename U, U (*RetainFunction)(U), void (*ReleaseFunction)(U)>
class RefCounted
{
public:
    RefCounted(const T& t = T())
        : value(t)
    {
    }
    RefCounted(RefCounted&& other)
        : value(other.value)
    {
        other.value = T();
    }
    RefCounted(const RefCounted& other)
        : value(other.value)
    {
        if (value)
        {
            RetainFunction(value);
        }
    }
    ~RefCounted()
    {
        if (value)
        {
            ReleaseFunction(value);
        }
    }
    operator T() const
    {
        return value;
    }
    void swap(RefCounted &other) noexcept
    {
        std::swap(value, other.value);
    }
    RefCounted &operator=(const RefCounted& other)
    {
        RefCounted copy(other);
        swap(copy);
        return *this;
    }
    RefCounted &operator=(RefCounted&& other)
    {
        RefCounted moved(std::move(other));
        swap(moved);
        return *this;
    }
    T *operator&()
    {
        return &value;
    }

protected:
    T value;
};


template <typename T>
class CFType : public RefCounted<T, CFTypeRef, CFRetain, CFRelease>
{
public:
    using RefCounted<T, CFTypeRef, CFRetain, CFRelease>::RefCounted;

    template <typename X> X as() const
    {
        return reinterpret_cast<X>(this->value);
    }

    static CFType constructFromGet(const T& t)
    {
        if (t)
        {
            CFRetain(t);
        }
        return CFType<T>(t);
    }
};

}
}

#endif // MAC_UTIL_H
