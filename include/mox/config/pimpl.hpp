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

#ifndef PIMPL_HPP
#define PIMPL_HPP

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr.get()); } \
    friend class Class##Private;

#define DECLARE_PRIVATE_D(Dptr, Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(Dptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(Dptr); } \
    friend class Class##Private;

#define DECLARE_PUBLIC(Class)                                    \
    public: \
    static Class##Private* get(Class& object) { return object.d_func(); } \
    static const Class##Private* cget(const Class& object) { return object.d_func(); } \
    static Class##Private* get(const Class& object) { return const_cast<Class*>(&object)->d_func(); } \
    static Class##Private* get(std::shared_ptr<Class> object) { return object->d_func(); } \
    static const Class##Private* get(std::shared_ptr<const Class> object) { return object->d_func(); } \
    inline Class* p_func() { return static_cast<Class *>(p_ptr); } \
    inline const Class* p_func() const { return static_cast<const Class *>(p_ptr); } \
    friend class Class;

#define D_PTR(Class) \
    Class##Private * const d = d_func()

#define P_PTR(Class) \
    Class * const p = p_func()

#endif // PIMPL_HPP
