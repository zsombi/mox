#ifndef PROPERTY_DECL_HPP
#define PROPERTY_DECL_HPP

#include <memory>

namespace mox
{

class PropertyType;
class PropertyValueProvider;

using PropertyValueProviderSharedPtr = std::shared_ptr<PropertyValueProvider>;

} // namespace mox

#endif // PROPERTY_DECL_HPP
