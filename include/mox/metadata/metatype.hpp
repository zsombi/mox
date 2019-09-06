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

#ifndef METATYPE_HPP
#define METATYPE_HPP

#include <any>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <string_view>

#include <mox/utils/globals.hpp>
#include <mox/utils/type_traits.hpp>

namespace mox
{

struct MetatypeDescriptor;

/// Defines the type identifier. User defined types are registered in the
/// user area, right after UserType.
enum class Metatype : int
{
    Invalid = -1,
    // void is a weirdo type
    Void = 0,
    Bool,
    Char,
    Byte,
    Short,
    Word,
    Int,
    UInt,
    Long,
    ULong,
    Int64,
    UInt64,
    Float,
    Double,
    NumericMax = Double, // Identifies the maximum of the numeric types.
    //
    String,
    Literal,
    // Pointer types
    VoidPtr,
    BytePtr,
    IntPtr,
    MetaObject,
    MetaObjectPtr,
    // All user types to be installed here
    UserType
};
ENABLE_ENUM_OPERATORS(Metatype)

/// Base type of all Mox arguments.
typedef std::any ArgumentBase;

/// Metatype converters
/// \{
/// Base converter.
struct MOX_API MetatypeConverter
{
    typedef ArgumentBase (*ConverterFunction)(const MetatypeConverter& /*converter*/, const void*/*value*/);

    /// Constructs a converter with a converter function.
    explicit MetatypeConverter(ConverterFunction function)
        : convert(function)
    {
    }
    explicit MetatypeConverter() = default;
    ConverterFunction convert = nullptr;

    DISABLE_COPY(MetatypeConverter)
};
typedef std::unique_ptr<MetatypeConverter> MetatypeConverterPtr;

class MOX_API bad_conversion : std::exception
{
public:
    explicit bad_conversion(Metatype from, Metatype to);
    virtual const char* what() const _NOEXCEPT override;
private:
    std::string m_message;
};

/// \}

namespace registrar
{
/// Finds a MetatypeDescriptor associated to the \a rtti.
/// \return nullptr if the \e rtti does not have any associated MetatypeDescriptor registered.
const MetatypeDescriptor* findMetatypeDescriptor(const std::type_info& rtti);

/// Finds a Metatype associated to the \a rtti.
/// \return The metatype identifier of the RTTI.
Metatype findMetatype(const std::type_info& rtti);

/// Registers a MetatypeDescriptor associated to the \a rtti.
/// \param rtti The type info of the type to register.
/// \param isEnum True if the type defines an enum.
/// \param isClass True if the type is a class.
/// \param isPointer True if the type is a pointer.
/// \param name Optional, the name of the metatype to override the default RTTI type name.
/// \return the MetatypeDescriptor associated to the \e rtti.
Metatype tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass, bool isPointer, std::string_view name);

template <typename T>
const std::type_info& remove_cv();

/// Registers a \a converter that converts a value from \a fromType to \a toType.
bool registerConverter(MetatypeConverterPtr&& converter, Metatype fromType, Metatype toType);

/// Look for the converter that converts a type between \a from and \a to.
/// \param from The source type.
/// \param to The destination type.
/// \return The converter found that converts a value between \a from and to \a to types.
/// nullptr is returned if there is no converter found to convert between the two metatypes.
MetatypeConverter* findConverter(Metatype from, Metatype to);

} // namespace registrar

/// Returns the metatype identifier of the given type. The function asserts if
/// the type is not registered in the metatype system.
/// Example:
/// \code
/// Metatype type = metaType<int*>();
/// \endcode
template <typename Type>
Metatype metaType();

template <typename Type>
const MetatypeDescriptor& metatypeDescriptor();

/// Registers a Type into the Mox metatype subsystem. The function returns the
/// Metatype identifier registered.
/// \return The Metatype handler of the Type.
template <typename Type>
Metatype registerMetaType(std::string_view name = "");

/// Registers a converter function that converts a value between two distinct types.
/// Returns \e true if the converter is registered with success, \e false otherwise.
/// A converter registration fails if Mox already has a converter for the desired types.
template <typename From, typename To, typename Function>
bool registerConverter(Function function);

/// Registers a converter method that converts the instance of the class that holds
/// the method to a given type.
/// Returns \e true if the converter is registered with success, \e false otherwise.
/// A converter registration fails if Mox already has a converter for the desired types.
template <typename From, typename To>
bool registerConverter(To (From::*function)() const);

/// Defines the type of an argument. Callable holds the argument descriptors for the return type aswell
/// as for the callable arguments.
struct ArgumentDescriptor
{
    /// Tye metatype of the argument.
    const Metatype type = Metatype::Invalid;
    /// \e true if the argument is a reference, \e false if not.
    const bool isReference = false;
    /// \e true if the argument is a const, \e false if not.
    const bool isConst = false;

    /// Constructor.
    ArgumentDescriptor() = default;
    ArgumentDescriptor(Metatype type, bool ref, bool c)
        : type(type)
        , isReference(ref)
        , isConst(c)
    {
    }

    template <typename Type>
    ArgumentDescriptor(Type)
        : type(metaType<Type>())
        , isReference(std::is_reference<Type>())
        , isConst(std::is_const<Type>())
    {
    }

    /// Returns the argument descriptor for the \e Type.
    /// \return The argument descriptor for the \e Type.
    template <typename Type>
    static ArgumentDescriptor&& get()
    {
        return std::move(ArgumentDescriptor{
                             metaType<Type>(),
                             std::is_reference<Type>(),
                             std::is_const<Type>()
                         });
    }

    /// Tests whether an \a other argument descriptor is compatible with this.
    bool invocableWith(const ArgumentDescriptor& other) const;

    /// Comparison operator, compares an \a other argument descriptor with this.
    bool operator==(const ArgumentDescriptor& other) const
    {
        return (other.type == type) &&
                other.isReference == isReference &&
                other.isConst == isConst;
    }
};

} // mox

#include <mox/metadata/detail/metatype_impl.hpp>


#endif // METATYPE_HPP
