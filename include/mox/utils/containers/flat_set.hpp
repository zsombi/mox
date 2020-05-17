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

#ifndef FLAT_SET_HPP
#define FLAT_SET_HPP

#include <algorithm>
#include <vector>

namespace mox
{

/// FlatSet is inspired from boost flat_set. It is a Sorted- and Unique Associative Container that stores
/// values of Key, without storing two values that are the same. It is similar to std::set, but implemented
/// as a sorted std::vector, not storing any overhead unlike std::set does.
/// \tparam Key The key type to store.
/// \tparam Compare The comparator.
/// \tparam Allocator The default allocator.
template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
class FlatSet
{
public:
    using ContainerType = std::vector<Key, Allocator>;
    using ConstIterator = typename ContainerType::const_iterator;
    using Iterator = typename ContainerType::iterator;
    using ConstReverseIterator = typename ContainerType::const_reverse_iterator;
    using ReverseIterator = typename ContainerType::reverse_iterator;

    /// Constructor.
    explicit FlatSet() = default;
    /// Destructor.
    ~FlatSet() = default;

    /// Construct a set from a container by copying the data from a portion of a container
    /// into the set.
    /// \tparam InputIterator the iterator type of the container.
    /// \param first The iterator pointing to the first element to copy into the set.
    /// \param last The iterator pointing to the last element to copy into the set.
    template <class InputIterator>
    explicit FlatSet(InputIterator first, InputIterator last)
    {
        auto invokeInsert = [this](const Key& value) { insert(value); };
        std::for_each(first, last, invokeInsert);
    }

    /// Construct a set from a container by copying the data from a container into the set.
    /// \tparam Container The type of the container.
    /// \param first The iterator pointing to the first element to copy into the set.
    /// \param last The iterator pointing to the last element to copy into the set.
    template <class Container>
    explicit FlatSet(const Container& container)
    {
        auto invokeInsert = [this](const Key& value) { insert(value); };
        std::for_each(container.begin(), container.end(), invokeInsert);
    }

    /// Test if the set is empty.
    /// \return If the set is empty, returns \e true, otherwise \e false.
    bool empty() const
    {
        return m_container.empty();
    }

    /// Returns the size of a set.
    /// \return The size of a set.
    size_t size() const
    {
        return m_container.size();
    }

    /// Returns a reference to the element of a set at specified \a position. No bounds checking
    ///  is performed.
    /// \param position The position of the element to return.
    /// \return The reference to the requested element.
    Key& operator[](size_t position)
    {
        return m_container[position];
    }

    /// Returns a const reference to the element of a set at specified \a position. No bounds
    /// checking is performed.
    /// \param position The position of the element to return.
    /// \return The const reference to the requested element.
    const Key& operator[](size_t position) const
    {
        return m_container[position];
    }

    /// Inserts a \a value into the set, if the set doesn't already include the value.
    /// \param value The value to insert.
    /// \return The iterator pointing to the position the value is inserted.
    auto insert(const Key& value)
    {
        auto pos = std::lower_bound(m_container.cbegin(), m_container.cend(), value, Compare());
        if ((pos != end()) && (value == *pos))
        {
            return end();
        }
        return m_container.insert(pos, value);
    }

    /// Inserts and moves a \a value into the set, if the set doesn't already include the value.
    /// \param value The value to insert.
    /// \return The iterator pointing to the position the value is inserted.
    auto insert(Key&& value)
    {
        ConstIterator pos = std::lower_bound(m_container.cbegin(), m_container.cend(), value, Compare());
        if ((pos != end()) && (value == *pos))
        {
            return end();
        }
        return m_container.insert(pos, std::move(value));
    }

    /// Removes the value from the set, if the set contains the value.
    /// \param value The value to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(const Key& value)
    {
        return m_container.erase(find(value));
    }

    /// Removes the elements of a set in the range [first, last).
    /// \param first The iterator pointing to the first element to remove.
    /// \param last The iterator pointing to the last element to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(Iterator first, Iterator last)
    {
        return m_container.erase(first, last);
    }

    /// Removes the elements of a set in the range [first, last).
    /// \param first The const iterator pointing to the first element to remove.
    /// \param last The const iterator pointing to the last element to remove.
    /// \return The iterator pointing to the value next to the removed value.
    auto erase(ConstIterator first, ConstIterator last)
    {
        return m_container.erase(first, last);
    }

    /// Find a value in a set.
    /// \param value The value to find.
    /// \return The iterator pointing to the position where the value is found. If the value is not found,
    /// return past-end-iterator.
    /// \see end()
    auto find(const Key& value)
    {
        auto first = std::lower_bound(begin(), end(), value, Compare());
        return (!(first == end()) && !Compare()(value, *first)) ? first : end();
    }

    /// Checks whether the set contains the value.
    /// \param value The value to find.
    /// \return If the flat set contains the value, returns \e true, otherwise \e false.
    bool contains(const Key& value)
    {
        auto it = find(value);
        return (it != end());
    }

    /// Returns an iterator to the beginning of the set.
    Iterator begin()
    {
        return m_container.begin();
    }
    /// Returns a const iterator to the beginning of the set.
    ConstIterator cbegin() const
    {
        return m_container.cbegin();
    }
    /// Returns a reverse iterator to the end of the set.
    ReverseIterator rbegin()
    {
        return m_container.rbegin();
    }
    /// Returns a const reverse iterator to the end of the set.
    ConstReverseIterator crbegin() const
    {
        return m_container.crbegin();
    }
    /// Returns an iterator to the end of the set.
    Iterator end()
    {
        return m_container.end();
    }
    /// Returns a const iterator to the end of the set.
    ConstIterator cend() const
    {
        return m_container.cend();
    }
    /// Returns a reverse iterator to the beginning of the set.
    ReverseIterator rend()
    {
        return m_container.rend();
    }
    /// Returns a const reverse iterator to the beginning of the set.
    ConstReverseIterator crend() const
    {
        return m_container.crend();
    }

private:
    ContainerType m_container;
};

/// Range erase free function specialized on FlatSet.
/// \tparam KeyType
/// \tparam Comparator
/// \tparam Allocator
/// \tparam ValueType
/// \param set The set from where you want to erase a value.
/// \param value The value to erase.
template <typename KeyType, typename Comparator, typename Allocator, typename ValueType>
void erase(FlatSet<KeyType, Comparator, Allocator>& set, const ValueType& value)
{
    set.erase(std::remove(set.begin(), set.end(), value), set.end());
}

} // namespace mox

#endif // FLAT_SET_HPP
