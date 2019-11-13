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

#ifndef FLAT_MAP_HPP
#define FLAT_MAP_HPP

#include <algorithm>
#include <vector>

#include <iostream>

namespace mox
{

/// FlatMap is inspired from boost flat_map. It is a Sorted- and Unique Associative Container that stores
/// values of Key and Value pairs, without storing two values that are the same. It is similar to std::map,
/// but implemented as a sorted std::vector, not storing any overhead unlike std::map does.
/// \tparam Key The key type to store.
/// \tparam Value The value type to store.
/// \tparam Compare The comparator.
/// \tparam Allocator The default allocator.
template <typename Key, typename Value, typename Comparator = std::less<Key>, typename Allocator = std::allocator<std::pair<Key, Value>>>
class FlatMap
{
public:
    using ElementType = std::pair<Key, Value>;
    using ContainerType = std::vector<ElementType, Allocator>;
    using ConstIterator = typename ContainerType::const_iterator;
    using Iterator = typename ContainerType::iterator;
    using ConstReverseIterator = typename ContainerType::const_reverse_iterator;
    using ReverseIterator = typename ContainerType::reverse_iterator;

    /// Constructor.
    explicit FlatMap() = default;
    /// Destructor.
    ~FlatMap() = default;

    /// Construct a map from a container by copying the data from a portion of a container
    /// into the map. The container must contain pairs of (Key, Value) type.
    /// \tparam InputIterator the iterator type of the container.
    /// \param first The iterator pointing to the first element to copy into the map.
    /// \param last The iterator pointing to the last element to copy into the map.
    template <class InputIterator>
    explicit FlatMap(InputIterator first, InputIterator last)
    {
        auto invokeInsert = [this](const ElementType& value) { insert(value); };
        std::for_each(first, last, invokeInsert);
    }

    /// Construct a map from a container by copying the data from a container into the map.
    /// \tparam Container The type of the container.
    /// \param first The iterator pointing to the first element to copy into the map.
    /// \param last The iterator pointing to the last element to copy into the map.
    template <class Container>
    explicit FlatMap(const Container& container)
    {
        auto invokeInsert = [this](const ElementType& value) { insert(value); };
        std::for_each(container.begin(), container.end(), invokeInsert);
    }

    /// Test if the map is empty.
    /// \return If the map is empty, returns \e true, otherwise \e false.
    bool empty() const
    {
        return m_container.empty();
    }

    /// Returns the size of a map.
    /// \return The size of a map.
    size_t size() const
    {
        return m_container.size();
    }

    /// Returns a reference to the element at specified \a position. No bounds checking is performed.
    /// \param position The position of the element to return.
    /// \return The reference to the requested element.
    auto& operator[](size_t position)
    {
        return m_container[position];
    }

    /// Returns a const reference to the element at specified \a position. No bounds checking is performed.
    /// \param position The position of the element to return.
    /// \return The const reference to the requested element.
    const auto& operator[](size_t position) const
    {
        return m_container[position];
    }

    /// Inserts a \a value into the container, if the container doesn't already include the value.
    /// \param value The pair of (key, value) to insert.
    /// \return The iterator pointing to the position the value is inserted.
    auto insert(const ElementType& value)
    {
        auto pos = std::lower_bound(m_container.cbegin(), m_container.cend(), value, ElementComparator());
        if ((pos != end()) && (value.first == pos->first))
        {
            return end();
        }
        return m_container.insert(pos, value);
    }

    /// Inserts and moves a \a value into the container, if the container doesn't already include the value.
    /// \param value The pair of (key, value) to insert.
    /// \return The iterator pointing to the position the value is inserted.
    auto insert(ElementType&& value)
    {
        auto pos = std::lower_bound(m_container.cbegin(), m_container.cend(), value, ElementComparator());
        if ((pos != end()) && (value.first == pos->first))
        {
            return end();
        }
        return m_container.insert(pos, std::forward<ElementType>(value));
    }

    /// Removes the value from the map, if the map contains the value.
    /// \param value The value to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(const Key& key)
    {
        return m_container.erase(find(key));
    }

    /// Removes the elements in the range [first, last).
    /// \param first The iterator pointing to the first element to remove.
    /// \param last The iterator pointing to the last element to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(Iterator first, Iterator last)
    {
        return m_container.erase(first, last);
    }

    /// Removes the elements in the range [first, last).
    /// \param first The const iterator pointing to the first element to remove.
    /// \param last The const iterator pointing to the last element to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(ConstIterator first, ConstIterator last)
    {
        return m_container.erase(first, last);
    }

    /// Find a value in a mapt.
    /// \param value The value to find.
    /// \return The iterator pointing to the position where the value is found. If the value is not found,
    /// return past-end-iterator.
    /// \see end()
    auto find(const Key& key)
    {
        auto first = std::lower_bound(begin(), end(), key, HybridComparator());
        return (!(first == end()) && !Comparator()(key, first->first)) ? first : end();
    }
    auto find(const Key& key) const
    {
        auto first = std::lower_bound(cbegin(), cend(), key, HybridComparator());
        return (!(first == cend()) && !Comparator()(key, first->first)) ? first : cend();
    }

    /// Checks whether the map contains the value.
    /// \param value The value to find.
    /// \return If the map contains the value, returns \e true, otherwise \e false.
    bool contains(const Key& key)
    {
        auto it = find(key);
        return (it != end());
    }

    /// Returns an iterator to the beginning of the flat set.
    Iterator begin()
    {
        return m_container.begin();
    }
    /// Returns a const iterator to the beginning of the flat set.
    ConstIterator cbegin() const
    {
        return m_container.cbegin();
    }
    /// Returns a reverse iterator to the end of the flat set.
    ReverseIterator rbegin()
    {
        return m_container.rbegin();
    }
    /// Returns a const reverse iterator to the end of the flat set.
    ConstReverseIterator crbegin() const
    {
        return m_container.crbegin();
    }
    /// Returns an iterator to the end of the flat set.
    Iterator end()
    {
        return m_container.end();
    }
    /// Returns a const iterator to the end of the flat set.
    ConstIterator cend() const
    {
        return m_container.cend();
    }
    /// Returns a reverse iterator to the beginning of the flat set.
    ReverseIterator rend()
    {
        return m_container.rend();
    }
    /// Returns a const reverse iterator to the beginning of the flat set.
    ConstReverseIterator crend() const
    {
        return m_container.crend();
    }

private:
    ContainerType m_container;

    struct ElementComparator : std::binary_function<ElementType, ElementType, bool>
    {
        Comparator cmp;
        bool operator()(const ElementType& l, const ElementType& r) const
        {
            return cmp(l.first, r.first);
        }
    };
    struct HybridComparator : std::binary_function<ElementType, Key, bool>
    {
        Comparator cmp;
        bool operator()(const ElementType& l, const Key& r) const
        {
            return cmp(l.first, r);
        }
    };
};

/// Range erase free function specialized on FlatMap.
/// \tparam KeyType
/// \tparam T
/// \tparam Comparator
/// \tparam Allocator
/// \tparam U
/// \param map The flat map from where you want to erase a value.
/// \param valuePair The value to erase.
template <typename KeyType, typename T, typename Comparator, typename Allocator, typename U>
void erase(FlatMap<KeyType, T, Comparator, Allocator>& map, const U& valuePair)
{
    map.erase(std::remove(map.begin(), map.end(), valuePair), map.end());
}

} // namespace mox

#endif // FLAT_MAP_HPP
