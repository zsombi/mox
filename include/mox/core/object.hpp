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

#include <mox/core/event_handling/event.hpp>
#include <mox/core/event_handling/run_loop_sources.hpp>
#include <mox/core/process/thread_data.hpp>
#include <mox/core/meta/properties.hpp>
#include <mox/utils/containers/shared_vector.hpp>
#include <mox/utils/containers/flat_map.hpp>

namespace mox
{

class Event;

class Object;
using ObjectSharedPtr = std::shared_ptr<Object>;
using ObjectWeakPtr = std::weak_ptr<Object>;

/// This class is the base class for Mox objects with metatype reflection. You can build an object hierarchy
/// by deriving your classes from Object and adding those as children to each other.
///
/// Provides event dispatching.
class MOX_API Object : public Lockable, public SlotHolder, public EventSource::EventDispatcher, public std::enable_shared_from_this<Object>
{
public:
    Property<std::string> objectName{*this};

    /// \name Event handling
    /// \{
    /// Events are dispatched to the event targets in two phases: tunneling and bubbling.
    /// In each phase the event is dispatched to objects that fall in between the root
    /// object and the target object of the event.
    ///
    /// During tunneling phase, the event is filtered out from being handled. This is realized
    /// by dispatching the event to the objects from root to the target object. The event is
    /// marked as handled before it is passed to the filterEvent() method. If the method returns
    /// \e true, the event is filtered out, and the event dispatching ends. If the method returns
    /// false, the event is unmarked from being handled and the event dispatching continues, till
    /// the target object is reached.
    ///
    /// If the event is not filtered out, the dispatching continues to the event handlers. This
    /// is realized by bubbling the events from the target object to the ascendants. The event is
    /// marked as handled before it is handed over to the handlers. This eases the event handler
    /// code to concentrate on the case when the event is not handled. If that happens, the event
    /// is bubbled to the closest ascendant event handler. This operation is repeated until an event
    /// handler consumes the event.

    /// The event filter type. The event filters return true if dispatching of the event is not desired
    /// after the handler call.
    using EventFilter = std::function<bool(Event&)>;
    /// The event handler type.
    using EventHandler = std::function<void(Event&)>;

    /// Event handler token, identifies an active event handler.
    class MOX_API EventToken : public std::enable_shared_from_this<EventToken>
    {
        friend class EventSource;
        ObjectWeakPtr m_target;
        EventType m_type;

    public:
        /// Constructor.
        explicit EventToken(EventType type, ObjectSharedPtr target);
        /// Destructor.
        virtual ~EventToken() = default;

        /// Returns the event type handled by the handler associated to the token.
        EventType getEventType() const
        {
            return m_type;
        }

        /// Returns the event handler host.
        /// \return The shared pointer
        auto getTarget() const
        {
            return m_target.lock();
        }

        /// Removes the event handler.
        void erase();

        /// Returns the validity state of the event handler token.
        /// \return If the event handler token is valid, returns \e true, otherwise \e false.
        bool isValid() const;
    };
    using EventTokenPtr = std::shared_ptr<EventToken>;

    /// Adds an event \a handler for an event \a type.
    /// \param type The event type.
    /// \param handler The handler function for the event.
    /// \return The token of the event handler.
    EventTokenPtr addEventHandler(EventType type, EventHandler handler);

    /// Adds an event \a filter for an event \a type.
    /// \param type The event type.
    /// \param filter The filter function for the event.
    /// \return The token of the event filter.
    EventTokenPtr addEventFilter(EventType type, EventFilter filter);

    /// \}

    /// Creates a shared pointer with Object. If a \a parent is specified, the object you create
    /// is added to this object as child. You can also add the created object to any object as
    /// child using addChild() method.
    static ObjectSharedPtr create(Object* parent = nullptr);
    /// Destructor.
    ~Object() override;

    /// Returns the pointer to the parent object.
    /// \return The pointer to the parent object, nullptr if the object has no parent set.
    Object* getParent() const;

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
    /// \throws ExceptionType::InvalidArgument if the \a child is not a child of this object.
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

    /// From EventSource::EventDispatcher
    void dispatchEvent(Event &event) override;

    /// Convenience template function, creates an object derived from Object, and adds it to the
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
    using TokenList = SharedVector<EventTokenPtr>;
    using Container = FlatMap<EventType, TokenList>;
    struct EventDispatcher
    {
        std::vector<ObjectSharedPtr> objects;
        explicit EventDispatcher(Object& target);
        bool processEventFilters(Event& event);
        void processEventHandlers(Event& event);
    };

    Container m_handlers;
    Container m_filters;
    ChildContainer m_children;
    mutable ThreadDataSharedPtr m_threadData;
    Object* m_parent = nullptr;

    friend class ThreadInterface;
    friend class Application;
};

LogLine& operator<<(LogLine& log, ObjectSharedPtr ptr);

} // mox

DECLARE_LOG_CATEGORY(object)

#endif // OBJECT_HPP
