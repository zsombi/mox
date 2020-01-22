#ifndef PROPERTY_DECL_HPP
#define PROPERTY_DECL_HPP

#include <memory>

namespace mox
{

class PropertyType;
class AbstractBinding;

using PropertyValueProviderSharedPtr = std::shared_ptr<AbstractBinding>;

} // namespace mox

#endif // PROPERTY_DECL_HPP
