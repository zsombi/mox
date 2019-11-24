#ifndef PROPERTY_DECL_HPP
#define PROPERTY_DECL_HPP

#include <memory>

namespace mox
{

class PropertyType;
class AbstractPropertyValueProvider;

using PropertyValueProviderSharedPtr = std::shared_ptr<AbstractPropertyValueProvider>;

} // namespace mox

#endif // PROPERTY_DECL_HPP
