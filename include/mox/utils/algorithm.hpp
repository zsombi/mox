#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <algorithm>
#include <vector>

namespace mox
{

template <typename Type, typename Allocator, typename VType>
void erase(std::vector<Type, Allocator>& v, const VType& value)
{
    v.erase(std::remove(v.begin(), v.end(), value), v.end());
}

/// Vector utility, removes the occurences for which the predicate gives affirmative result.
template <typename Type, typename Allocator, typename Predicate>
void erase_if(std::vector<Type, Allocator>& v, const Predicate& predicate)
{
    v.erase(std::remove_if(v.begin(), v.end(), predicate), v.end());
}

template <typename Container, typename Predicate>
void for_each(Container& c, Predicate predicate)
{
    std::for_each(c.begin(), c.end(), predicate);
}

template <typename Type, typename Allocator, typename VType>
auto find(std::vector<Type, Allocator>& v, const VType& value)
{
    return std::find(v.begin(), v.end(), value);
}

template <typename Container, typename Predicate>
auto find_if(Container& c, Predicate predicate)
{
    return std::find_if(c.begin(), c.end(), predicate);
}

template <typename Container, typename Predicate>
auto reverse_find_if(Container& c, Predicate predicate)
{
    return std::find_if(c.rbegin(), c.rend(), predicate);
}

} // mox

#endif // ALGORITHM_HPP
