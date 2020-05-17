/*
 * Copyright (C) 2017-2020 bitWelder
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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <mox/config/platform_config.hpp>
#include <mox/config/deftypes.hpp>
#include <mox/utils/type_traits/enum_operators.hpp>
#include <string_view>
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <vector>
#include <memory>

namespace mox
{

/// The log types.
enum class LogType : byte
{
    None = 0,
    Debug = 0x01,
    Warning = 0x02,
    Info = 0x04,
    Fatal = 0x08,
    All = 0x0F
};
ENABLE_ENUM_OPERATORS(LogType)

/// Logging category.
struct MOX_API LogCategory
{
    explicit LogCategory(std::string_view name);
    ~LogCategory() = default;

    std::string getName() const
    {
        return m_categoryName;
    }
    LogType getTypes() const
    {
        return m_types;
    }
    void setTypes(LogType types)
    {
        m_types = types | LogType::Fatal;
    }
    bool hasTypes(LogType types) const;

private:
    std::string m_categoryName;
    LogType m_types;
};

/// The LoggerInterface declares the interface for the loggers. Mox has two logger types,
/// the screen and the file logger.
class MOX_API LoggerInterface
{
public:
    /// Destructor.
    virtual ~LoggerInterface() = default;
    /// Logs a text.
    /// \param test The text to log.
    virtual bool log(LogCategory& category, LogType type, std::string_view heading, const std::string& text) = 0;
};
using LoggerInterfacePtr = std::unique_ptr<LoggerInterface>;

/// Screen logger, logs a text to the screen.
class MOX_API ScreenLogger : public LoggerInterface
{
public:
    /// Constructor.
    explicit ScreenLogger() = default;
    /// Override of LoggerInterface::log().
    bool log(LogCategory& category, LogType type, std::string_view heading, const std::string &text) override;
};

/// File logger, logs a text to a file.
class MOX_API FileLogger : public LoggerInterface
{
public:
    /// Constructor, logs text to a file.
    /// \param fileName The name of the file to log.
    /// \param append Tells the logger to append to the file if exists. If the file does not exist,
    /// the argument is ignored.
    explicit FileLogger(std::string_view fileName, bool append);
    /// Destructor, closes the log file.
    ~FileLogger() override;

    /// Override of LoggerInterface::log().
    bool log(LogCategory& category, LogType type, std::string_view heading, const std::string& text) override;

private:
    std::ofstream stream;
};

/// Logs a line to the current logger.
class MOX_API LogLine
{
    std::ostringstream m_data;
    std::string m_heading;
    LogCategory* m_category = nullptr;
    LogType m_logType;

public:
    /// Constructs the log line.
    LogLine(LogType type, const char* file, unsigned line, const char* function);
    /// Constructs the log line for a category.
    LogLine(std::string_view category, LogType type, const char* file, unsigned line, const char* function);
    /// Destructs the log line and flushes the content to the current logger.
    ~LogLine();

    /// Returns the enabled state of the line logger. A line logger is disabled if the category
    /// is disabled. Category-less line loggers are always enabled.
    /// \return The enabled state of the line logger.
    bool isEnabled() const
    {
        return m_category && m_category->hasTypes(m_logType);
    }

    /// Stream operators for default types.
    LogLine& operator<<(bool v);
    LogLine& operator<<(char v);
    LogLine& operator<<(signed int v);
    LogLine& operator<<(unsigned int v);
    LogLine& operator<<(size_t v);
    LogLine& operator<<(const char* v);
    LogLine& operator<<(std::string_view v);
    LogLine& operator<<(const std::string& v);
    LogLine& operator<<(intptr_t v);
    LogLine& operator<<(void* v);
    LogLine& operator<<(double v);
};

/// The logger manager.
class MOX_API Logger
{
public:
    /// Logs a text using the current logger interface.
    /// \param category The logging category.
    /// \param type The log type.
    /// \param text The text to log.
    static void log(LogCategory& category, LogType type, std::string_view heading, const std::string& text);

    /// Set the current logger interface.
    /// \param logger The logger to use.
    static void setLogger(LoggerInterfacePtr logger);

    /// Adds a log category to the logger.
    /// \param category The category descriptor to add.
    /// \return The category identifier.
    static size_t addCategory(LogCategory category);
    /// Find a log category by name.
    static LogCategory* findCategory(std::string_view category);
    /// Returns the category with the given \a id;
    static LogCategory& getCategory(size_t id);

    /// Sets the logging rules. The logging rules drive the log categories enabled togehther with the log
    /// types on the category.
    /// \param rules The log configuration rules.
    ////
    /// The rule string format:
    /// <rule>[:<rule>*]
    /// rule: <category_name>.<debug|warning|info|*>=<true|false>
    /// Example:
    /// \code default.warning=false:default.info=false:metacore.*=true
    /// \note You can configure the trace logs on an application using MOX_LOG_RULES environment variable.
    /// Also note that the fatal log type is not configurable, that log type is taken into use on any debug
    /// build.
    static void setRules(std::string rules);

private:
    Logger() = default;
    ~Logger() = default;
};

struct LogCategoryRegistrar
{
    LogCategoryRegistrar(const char* ctg)
        : _category(ctg)
    {
    }
    operator std::string_view() const
    {
        return _category;
    }

private:
    const char* _category;
};

} // mox

#if defined(MOX_ENABLE_LOGS)
#define DECLARE_LOG_CATEGORY(category) \
const mox::LogCategoryRegistrar logCategoryRegistrar_##category = {#category};

#define CATEGORY(c)         ::logCategoryRegistrar_##c

#define CTRACE(c, s)        mox::LogLine(CATEGORY(c), mox::LogType::Debug, __FILE__, __LINE__, __FUNCTION__) << s
#define CWARN(c, s)         mox::LogLine(CATEGORY(c), mox::LogType::Warning, __FILE__, __LINE__, __FUNCTION__) << s
#define CINFO(c, s)         mox::LogLine(CATEGORY(c), mox::LogType::Info, __FILE__, __LINE__, __FUNCTION__) << s
#define CFATAL(c, test, s)  if (!(test)) mox::LogLine(CATEGORY(c), mox::LogType::Fatal, __FILE__, __LINE__, __FUNCTION__) << s

#define TRACE(s)           mox::LogLine(mox::LogType::Debug, __FILE__, __LINE__, __FUNCTION__) << s
#define WARN(s)            mox::LogLine(mox::LogType::Warning, __FILE__, __LINE__, __FUNCTION__) << s
#define INFO(s)            mox::LogLine(mox::LogType::Info, __FILE__, __LINE__, __FUNCTION__) << s
#define FATAL(test, s)     if (!(test)) mox::LogLine(mox::LogType::Fatal, __FILE__, __LINE__, __FUNCTION__) << s

#else

#include <cstdlib>

#define DECLARE_LOG_CATEGORY(category)
#define CATEGORY(category)
#define CTRACE(category, s)
#define CWARN(category, s)
#define CINFO(category, s)
#define CFATAL(category, test, s)   if (!(test)) std::exit(EXIT_FAILURE)

#define TRACE(s)
#define WARN(s)
#define INFO(s)
#define FATAL(test, s)              if (!(test)) std::exit(EXIT_FAILURE)

#endif

#endif // LOGGER_HPP
