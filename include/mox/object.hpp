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

#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <mox/utils/globals.hpp>
#include <mox/metadata/metaobject.hpp>
#include <mox/event_handling/event_handler.hpp>
#include <mox/config/thread.hpp>
#include <mox/module/thread_data.hpp>

namespace mox
{

class Event;

class Object;
using ObjectSharedPtr = std::shared_ptr<Object>;
using ObjectWeakPtr = std::weak_ptr<Object>;

/// This class is the base class for Mox objects with metatype reflection. You can build an object hierarchy
/// by deriving your classes from Object and adding those as children to each other.
///
/// Derived from EventHandlingProvider, provides event handling mechanisms. See EventHandlingProvider for more
/// details on event handling.
class MOX_API Object : public MetaObject, public EventHandlingProvider, public std::enable_shared_from_this<Object>
{
public:
    /// The static metaclass of Object.
    struct StaticMetaClass : mox::StaticMetaClass<StaticMetaClass, Object, MetaObject>
    {
    };

    /// Creates a shared pointer with Object. If a \a parent is specified, the object you create
    /// is added to this object as child. You can also add the created object to any object as
    /// child using addChild() method.
    static ObjectSharedPtr create(Object* parent = nullptr);
    /// Destructor.
    ~Object() override;

    /// Returns the pointer to the parent object.
    /// \return The pointer to the parent object, nullptr if the object has no parent set.
    Object* parent() const;

    /// Add a \a child to this object as child.
    void addChild(Object& child);
    /// Removes the \a child object from this object's children.
    void removeChild(Object& child);
    /// Removes the child object at \a index from this object's children.
    void removeChildAt(size_t index);
    /// Returns the number of children in the object.
    /// \return The number of children of this object, 0 if the object has no children.
    size_t childCount() const;
    /// Returns the child index of a given \a child object.
    /// \return The index of the \a child object in the children list.
    /// \throws Callable::invalid_argument if the \a child is not a child of this object.
    size_t childIndex(const Object& child);
    /// Returns the child object at \a index.
    /// \return The child object at index, nullptr if there is no child at the specified index.
    ObjectSharedPtr childAt(size_t index);

    /// Specifies the traverse order.
    enum class TraverseOrder
    {
        /// NLR order
        PreOrder,
        /// LRN
        PostOrder,
        /// RLN
        InversePreOrder,
        /// NRL
        InversePostOrder
    };
    /// Specifies the visiting result
    enum class VisitResult
    {
        /// Aborts the traversal.
        Abort,
        /// Continues the traversal
        Continue,
        /// Continues the traversal on the siblings of the visited object.
        ContinueSibling
    };

    using VisitorFunction = std::function<VisitResult(Object&)>;
    VisitResult traverse(const VisitorFunction& visitor, TraverseOrder order);
    VisitResult traverseChildren(const VisitorFunction& visitor, TraverseOrder order);

    /// Returns the thread data of this object.
    ThreadDataSharedPtr threadData() const;

protected:
    /// Constructor.
    explicit Object();

    /// Removes all child objects of this Object.
    void removeChildren();

    /// Visitor function, moves this object and its child objects to the given thread.
    /// \param threadData The thread data where this object is moved.
    /// \return The visit result.
    virtual VisitResult moveToThread(ThreadDataSharedPtr threadData);

    /// Convenience template function, creates an obhect derived from Object, and adds it to the
    /// parent object.
    template <class Derived>
    static std::shared_ptr<Derived> createObject(Derived* newObject, Object* parent = nullptr)
    {
        std::shared_ptr<Derived> sharedNewObject(newObject);
        if (parent)
        {
            parent->addChild(*newObject);
        }
        return sharedNewObject;
    }

private:
    using ChildContainer = std::vector<ObjectSharedPtr>;

    Object* m_parent = nullptr;
    ChildContainer m_children;
    mutable ThreadDataSharedPtr m_threadData;
    friend class ThreadLoop;
};

} // mox

#endif // OBJECT_HPP
