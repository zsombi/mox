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

#include <string_view>
#include <array>
#include <vector>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <mox/core/meta/core/metatype.hpp>

#include <unordered_map>

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
    /// Converter for metatypes.
    class MOX_API Converter final
    {
    public:
        /// Constructs a converter with a converter function.
        Converter() = default;

        /// Copy constructor.
        Converter(const Converter& other);

        /// Move constructor.
        Converter(Converter&& rhs) noexcept;

        /// Destructor.
        ~Converter();

        /// Convert a value with the hosted constructor.
        /// \param value The value to convert.
        /// \return The converted value.
        const MetaValue convert(const void* value) const;

        /// Creates an explicit metatype converter.
        /// \tparam From The source type to convert from
        /// \tparam To The destination type to convert to.
        /// \return The metatype converter.
        template <typename From, typename To>
        static Converter fromExplicit();

        /// Creates a dynamic metatype converter. The types must be objects.
        /// \tparam From The source type to convert from
        /// \tparam To The destination type to convert to.
        /// \return The metatype converter.
        template <typename From, typename To>
        static Converter dynamicCast();

        /// Creates a metatype converter that uses a function to convert the types.
        /// \tparam From The source type to convert from
        /// \tparam To The destination type to convert to.
        /// \tparam Function The converter function type
        /// \param function The converter function, a functor or a lambda.
        /// \return The metatype converter.
        template <typename From, typename To, typename Function>
        static Converter fromFunction(Function function);

        /// Creates a metatype converter that uses a method to convert the types. The source is
        /// a class that holds the method, which converts the source to a destination type.
        /// \tparam From The source class to convert from.
        /// \tparam To The destination type to convert to.
        /// \param method The converter method.
        /// \return The metatype converter.
        template <class From, class To>
        static Converter fromMethod(To (From::*method)() const);

    private:
        using Storage = std::any;
        struct VTable
        {
            MetaValue (*convert)(const Storage&, const void*);
        };

        Storage m_storage;
        VTable* m_vtable = nullptr;

        explicit Converter(const Storage& storage, VTable* vtable);

        void swap(Converter& other);

        template <typename From, typename To>
        struct ExplicitConverter
        {
            static MetaValue convert(const Storage&, const void* value);
        };

        template <typename From, typename To>
        struct DynamicCast
        {
            static MetaValue convert(const Storage&, const void* value);
        };

        template <typename From, typename To, typename Function>
        struct FunctionConverter
        {
            static MetaValue convert(const Storage& fn, const void* value);
        };

        template <typename From, typename To>
        struct MethodConverter
        {
            static MetaValue convert(const Storage& storage, const void* value);
        };
    };


    /// Destructor.
    virtual ~MetatypeDescriptor() final;

    /// Checks whether the Type is a custom metatype.
    bool isCustomType();

    /// Returns the MetatypeDescriptor of a given type identifier.
    /// \param typeId The type identifier.
    /// \return The registered MetatypeDescriptor holding the typeId.
    static const MetatypeDescriptor& get(Metatype typeId);

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

    /// Return \e true if the type held by this MetatypeDescriptor is a pointer.
    bool isPointer() const;

    /// Returns the RTTI (Run-time Type Information) of the MetatypeDescriptor.
    const std::type_info* rtti() const;

    /// Find a converter that converts a value if this metatype into \a target metatype.
    /// \param target The target metatype.
    /// \return The pointer to the converter found, nullptr if no converter is found.
    const Converter* findConverterTo(Metatype target) const;

    /// Registers a converter to convert from \a fromType to \a toType.
    /// \param converter The converter to register.
    /// \param fromType The source type the converter converts a value from.
    /// \param toType The destination type the converter converts a value.
    /// \return If the converter is registered with success, returns \e true, otherwise \e false.
    static bool registerConverter(Converter&& converter, Metatype fromType, Metatype toType);

    /// Find a converter that converts a value from \a fromType, \a toType.
    /// \param fromType The source type.
    /// \param toType The destination type.
    /// \param The pointer to the converter that converts the types, nullptr if no converter
    /// is found.
    static const Converter* findConverter(Metatype fromType, Metatype toType);

private:
    /// MetatypeDescriptor constructor.
    explicit MetatypeDescriptor(std::string_view name, int id, const std::type_info& rtti, bool isEnum, bool isClass, bool isPointer);

    /// Adds a converter that converts this metatype to target.
    bool addConverter(Converter&& converter, Metatype target);

    using ConverterMap = std::unordered_map<Metatype, Converter>;

    ConverterMap m_converters;
    char* m_name = nullptr;
    const std::type_info* m_rtti = nullptr;
    Metatype m_id = Metatype::Invalid;
    bool m_isEnum:1;
    bool m_isClass:1;
    bool m_isPointer:1;

    friend struct MetaData;
    friend const MetatypeDescriptor* tryRegisterMetatype(const std::type_info &rtti, bool isEnum, bool isClass);

    DISABLE_COPY(MetatypeDescriptor)
    DISABLE_MOVE(MetatypeDescriptor)

    template <typename From, typename To>
    friend bool registerConverter();
    template <typename From, typename To, typename Function>
    friend bool registerConverter(Function function);
    template <typename From, typename To>
    friend bool registerConverter(To (From::*method)() const);

};

} // namespace mox

#include <mox/core/meta/core/detail/metatype_descriptor_impl.hpp>

#endif // METATYPE_DESCRIPTOR_HPP
