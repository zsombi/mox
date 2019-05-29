/*
 * Copyright (C) 2017-2018 bitWelder
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

#include <array>
#include <vector>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <mox/utils/globals.hpp>

namespace mox {

/// The MetaTypeDescriptor class extends the RTTI of the types in Mox. Provides information
/// about the type, such as constness, whether is a pointer or enum. It also stores
/// a fully qualified name of the type. The MetaTypeDescriptor is also used when comparing
/// arguments passed on invocation with the arguments of the metamethods.
///
/// The class is a standalone class and cannot be derived.
struct MOX_API MetaTypeDescriptor
{
public:
    /// Destructor.
    virtual ~MetaTypeDescriptor() final;

    /// Defines the type identifier. User defined types are registered in the
    /// user area, right after UserType.
    enum class TypeId : int
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
        String,
        MetaObject,
        VoidPtr,
        // All user types to be installed here
        UserType
    };

    /// Returns the registered MetaTypeDescriptor for the given type. The function asserts
    /// if the type is not registered in the metatype system.
    /// Example:
    /// \code
    /// const MetaTypeDescriptor* intPtr = MetaTypeDescriptor::value<int*>();
    /// \endcode
    template<typename Type>
    static const MetaTypeDescriptor& get();

    /// Returns the type identifier of the given type. The function asserts if
    /// the type is not registered in the metatype system.
    /// Example:
    /// \code
    /// MetaTypeDescriptor::TypeId type = MetaTypeDescriptor::id<int*>();
    /// \endcode
    template <typename Type>
    static TypeId typeId();

    template <typename Type>
    static bool isCustomType();

    /// Returns the MetaTypeDescriptor of a given type identifier.
    /// \param typeId The type identifier.
    /// \return The registered MetaTypeDescriptor holding the typeId.
    static const MetaTypeDescriptor& get(TypeId typeId);

    /// Checks whether this metatype is the supertype of the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is the supertype of the type, \e false if not.
    bool isSupertypeOf(const MetaTypeDescriptor& type) const;

    /// Checks whether this metatype is derived from the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is derived from the type, \e false if not.
    bool derivesFrom(const MetaTypeDescriptor& type) const;

    /// Returns \e true if the MetaTypeDescriptor holds a valid type.
    bool isValid() const;

    /// Returns \e true if the MetaTypeDescriptor holds the void value.
    /// \note void pointers are reported as a separate type.
    bool isVoid() const;

    /// Returns the type identifier held by the MetaTypeDescriptor.
    TypeId id() const;

    /// Returns the fully qualified name of the MetaTypeDescriptor.
    const char* name() const;

    /// Returns \e true if the type held by the MetaTypeDescriptor is an enumeration.
    bool isEnum() const;

    /// Returns \e true if the type held by this MetaTypeDescriptor is a class.
    bool isClass() const;

    /// Returns the RTTI (Run-time Type Information) of the MetaTypeDescriptor.
    const std::type_info* rtti() const;

    /// Finds a MetaTypeDescriptor associated to the \a rtti.
    /// \return nullptr if the \e rtti does not have any associated MetaTypeDescriptor registered.
    static const MetaTypeDescriptor* findMetaType(const std::type_info& rtti);

    /// Registers a Type into the Mox metatype subsystem. The function returns the
    /// MetaTypeDescriptor if already registered.
    /// \return The MetaTypeDescriptor handler of the Type.
    template <typename Type>
    static MetaTypeDescriptor::TypeId registerMetaType();

    /// Converters
    /// \{
    struct AbstractConverter : public std::enable_shared_from_this<AbstractConverter>
    {
        typedef bool (*ConverterFunction)(const AbstractConverter& /*converter*/, const void*/*from*/, void* /*to*/);
        explicit AbstractConverter(ConverterFunction function = nullptr) :
            m_convert(function)
        {
        }
        DISABLE_COPY(AbstractConverter)
        ConverterFunction m_convert;
    };
    typedef std::shared_ptr<AbstractConverter> AbstractConverterSharedPtr;

    template <typename From, typename To, typename Function>
    struct ConverterFunctor : public MetaTypeDescriptor::AbstractConverter
    {
        Function m_function;

        explicit ConverterFunctor(Function function) :
            MetaTypeDescriptor::AbstractConverter(convert),
            m_function(function)
        {
        }

        static bool convert(const MetaTypeDescriptor::AbstractConverter& converter, const void* from, void* to)
        {
            const From* in = reinterpret_cast<const From*>(from);
            To* out = reinterpret_cast<To*>(to);
            const ConverterFunctor& that = reinterpret_cast<const ConverterFunctor&>(converter);
            *out = that.m_function(*in);
            return true;
        }
    };

    template <typename From, typename To, typename Function>
    static bool registerConverter(Function function)
    {
        const MetaTypeDescriptor::TypeId fromType = MetaTypeDescriptor::typeId<From>();
        const MetaTypeDescriptor::TypeId toType = MetaTypeDescriptor::typeId<To>();
        AbstractConverterSharedPtr converter = make_polymorphic_shared<AbstractConverter, ConverterFunctor<From, To, Function> >(function);
        return registerConverterFunction(converter, fromType, toType);
    }
    /// Look for the converter that converts a type between \a from and \a to.
    /// \param from The source type.
    /// \param to The destination type.
    /// \return The converter found, nullptr if there is no converter registered for the type.
    static AbstractConverterSharedPtr findConverter(TypeId from, TypeId to);
    /// \}

private:
    /// MetaTypeDescriptor constructor.
    explicit MetaTypeDescriptor(const char* name, int id, const std::type_info& rtti, bool isEnum, bool isClass);
    /// Registers a MetaTypeDescriptor associated to the \a rtti.
    /// \param rtti The type info of the type to register.
    /// \param isEnum True if the type defines an enum.
    /// \return the MetaTypeDescriptor associated to the \e rtti.
    static const MetaTypeDescriptor& newMetatype(const std::type_info &rtti, bool isEnum, bool isClass);

    /// Registers a \a converter that converts a value from \a fromType to \a toType.
    static bool registerConverterFunction(AbstractConverterSharedPtr converter, TypeId fromType, TypeId toType);
    /// Removes a converter between \a fromType and \a toType that is registered for the \a forType.
    static void unregisterConverterFunction(TypeId fromType, TypeId toType);

    char* m_name{nullptr};
    const std::type_info* m_rtti{nullptr};
    TypeId m_id{TypeId::Invalid};
    bool m_isEnum:1;
    bool m_isClass:1;

    friend class MetaData;
    DISABLE_COPY(MetaTypeDescriptor)
    DISABLE_MOVE(MetaTypeDescriptor)
};

} // namespace mox

#include <mox/metadata/detail/metatype_impl.hpp>

#endif // METATYPE_H
