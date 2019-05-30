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

#ifndef METATYPE_DESCRIPTOR_HPP
#define METATYPE_DESCRIPTOR_HPP

#include <array>
#include <vector>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <mox/utils/globals.hpp>
#include <mox/metadata/metatype.hpp>

namespace mox {

/// The MetatypeDescriptor class extends the RTTI of the types in Mox. Provides information
/// about the type, such as constness, whether is a pointer or enum. It also stores
/// a fully qualified name of the type. The MetatypeDescriptor is also used when comparing
/// arguments passed on invocation with the arguments of the metamethods.
///
/// The class is a standalone class and cannot be derived.
struct MOX_API MetatypeDescriptor
{
public:
    /// Destructor.
    virtual ~MetatypeDescriptor() final;

    /// Checks whether the Type is a custom metatype.
    bool isCustomType();

    /// Returns the MetatypeDescriptor of a given type identifier.
    /// \param typeId The type identifier.
    /// \return The registered MetatypeDescriptor holding the typeId.
    static const MetatypeDescriptor& get(Metatype typeId);

    /// Checks whether this metatype is the supertype of the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is the supertype of the type, \e false if not.
    bool isSupertypeOf(const MetatypeDescriptor& type) const;

    /// Checks whether this metatype is derived from the \a type passed as argument.
    /// Both this type and the passed metatype must be class types.
    /// \return \e true if this type is derived from the type, \e false if not.
    bool derivesFrom(const MetatypeDescriptor& type) const;

    /// Returns \e true if the MetatypeDescriptor holds a valid type.
    bool isValid() const;

    /// Returns \e true if the MetatypeDescriptor holds the void value.
    /// \note void pointers are reported as a separate type.
    bool isVoid() const;

    /// Returns the type identifier held by the MetatypeDescriptor.
    Metatype id() const;

    /// Returns the fully qualified name of the MetatypeDescriptor.
    const char* name() const;

    /// Returns \e true if the type held by the MetatypeDescriptor is an enumeration.
    bool isEnum() const;

    /// Returns \e true if the type held by this MetatypeDescriptor is a class.
    bool isClass() const;

    /// Returns the RTTI (Run-time Type Information) of the MetatypeDescriptor.
    const std::type_info* rtti() const;

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
    struct ConverterFunctor : public MetatypeDescriptor::AbstractConverter
    {
        Function m_function;

        explicit ConverterFunctor(Function function) :
            MetatypeDescriptor::AbstractConverter(convert),
            m_function(function)
        {
        }

        static bool convert(const MetatypeDescriptor::AbstractConverter& converter, const void* from, void* to)
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
        const Metatype fromType = metaType<From>();
        const Metatype toType = metaType<To>();
        AbstractConverterSharedPtr converter = make_polymorphic_shared<AbstractConverter, ConverterFunctor<From, To, Function> >(function);
        return registerConverterFunction(converter, fromType, toType);
    }
    /// Look for the converter that converts a type between \a from and \a to.
    /// \param from The source type.
    /// \param to The destination type.
    /// \return The converter found, nullptr if there is no converter registered for the type.
    static AbstractConverterSharedPtr findConverter(Metatype from, Metatype to);
    /// \}

private:
    /// MetatypeDescriptor constructor.
    explicit MetatypeDescriptor(const char* name, int id, const std::type_info& rtti, bool isEnum, bool isClass);

    /// Registers a \a converter that converts a value from \a fromType to \a toType.
    static bool registerConverterFunction(AbstractConverterSharedPtr converter, Metatype fromType, Metatype toType);
    /// Removes a converter between \a fromType and \a toType that is registered for the \a forType.
    static void unregisterConverterFunction(Metatype fromType, Metatype toType);

    char* m_name{nullptr};
    const std::type_info* m_rtti{nullptr};
    Metatype m_id{Metatype::Invalid};
    bool m_isEnum:1;
    bool m_isClass:1;

    friend class MetaData;
    friend const MetatypeDescriptor* tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass);
    template <typename Type>
    friend Metatype registerMetaType();

    DISABLE_COPY(MetatypeDescriptor)
    DISABLE_MOVE(MetatypeDescriptor)
};

} // namespace mox

#endif // METATYPE_DESCRIPTOR_HPP
