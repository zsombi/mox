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

#if defined(MOX_ENABLE_LOGS)

#include <logger_p.hpp>
#include <mox/utils/locks.hpp>
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>

#ifdef ANDROID
    #include <android/log.h>
    #define TAG "MoxFW"
#endif

//DECLARE_LOG_CATEGORY(default)

namespace mox
{

/******************************************************************************
 * LoggerData
 */
LoggerData::LoggerData()
    : m_logger(std::make_unique<ScreenLogger>())
{
    auto env = getenv("MOX_LOG_RULES");
    if (env)
    {
        setRules(env);
    }
    else
    {
        setRules("default.*=true");
    }
    g_logger = this;
}

LoggerData::~LoggerData()
{
    g_logger = nullptr;
}

LoggerData& LoggerData::get()
{
    assert(g_logger);
    return *g_logger;
}

LoggerData* LoggerData::find()
{
    return g_logger;
}

void LoggerData::log(const std::string& text)
{
    lock_guard lock(m_mutex);
    m_logger->log(text);
}

void LoggerData::setLogger(LoggerInterfacePtr logger)
{
    m_logger = std::move(logger);
}

size_t LoggerData::addCategory(LogCategory category)
{
    auto finder = [&category](auto& c)
    {
        return c.getName() == category.getName();
    };
    auto it = std::find_if(m_categories.begin(), m_categories.end(), finder);
    if (it != m_categories.end())
    {
        return std::distance(m_categories.begin(), it);
    }

    m_categories.emplace_back(std::move(category));
    return m_categories.size() - 1u;
}

LogCategory* LoggerData::findCategory(std::string_view category)
{
    auto finder = [&category](auto& c)
    {
        return c.getName() == category;
    };
    auto it = std::find_if(m_categories.begin(), m_categories.end(), finder);
    return (it != m_categories.end())
            ? &(*it)
            : nullptr;
}

LogCategory& LoggerData::getCategory(size_t id)
{
    return m_categories[id];
}

void LoggerData::setRules(std::string rules)
{
    if (rules.empty())
    {
        return;
    }

    rules += ':';
    while (!rules.empty())
    {
        auto limitatorPos = rules.find(':');
        auto rule = rules.substr(0, limitatorPos);
        rules.erase(0, limitatorPos + 1);
        setRule(rule);
    }
}

void LoggerData::setRule(std::string rule)
{
    if (rule.empty())
    {
        return;
    }

    auto pos = rule.find('.');
    auto category = rule.substr(0, pos);
    rule.erase(0, pos + 1);

    pos = rule.find('=');
    auto logType = rule.substr(0, pos);
    rule.erase(0, pos + 1);

    if (logType != "debug" && logType != "warning" && logType != "info" && logType != "fatal" && logType != "*")
    {
        std::cerr << "Warning: invalid log type given: " << logType << std::endl;
        return;
    }

    if (rule != "true" && rule != "false")
    {
        std::cerr << "Warning: invalid rule value given: " << rule << std::endl;
        return;
    }

    bool enable = (rule == "true");
    LogType types = LogType::Fatal;
    if (logType == "*" && enable)
    {
        types = LogType::All;
    }
    else if (logType == "debug" && enable)
    {
        types = LogType::Debug;
    }
    else if (logType == "warning" && enable)
    {
        types = LogType::Warning;
    }
    else if (logType == "info" && enable)
    {
        types = LogType::Info;
    }

    auto id = addCategory(LogCategory(category));
    auto& pcategory = m_categories[id];
    pcategory.setTypes(types);
}

/******************************************************************************
 * ScreenLogger
 */
bool ScreenLogger::log(const std::string& text)
{
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, TAG, text.c_str());
#else
    clock_t rawTime = clock();
    std::string line = "[tid<> "
            + std::to_string(rawTime) + "] " + text;
    std::cout << line << std::endl;
#endif
    return true;
}

/******************************************************************************
 * FileLogger
 */
FileLogger::FileLogger(std::string_view fileName, bool append)
    : stream(fileName.data(), std::ofstream::out | (append ? std::ofstream::app : std::ofstream::trunc))
{
    stream << "**************************************************\n";
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    stream << "Mox logging " << ctime(&now);
    stream << "LoggerType: " << getenv("MOX_LOG_RULES") << std::endl;
    stream << "********START OF LOG******************************\n\n";
}

FileLogger::~FileLogger()
{
    stream << "\n********END OF LOG********************************\n\n";
    stream.close();
}

bool FileLogger::log(const std::string& text)
{
    clock_t rawTime = clock();
    stream << "[tid<> " << std::to_string(rawTime) + "] "
           << text << std::endl;
    return true;
}

/******************************************************************************
 * Logger
 */
void Logger::log(const std::string& text)
{
    LoggerData::get().log(text);
}

void Logger::setLogger(LoggerInterfacePtr logger)
{
    LoggerData::get().setLogger(std::move(logger));
}

size_t Logger::addCategory(LogCategory category)
{
    return LoggerData::get().addCategory(std::move(category));
}

LogCategory* Logger::findCategory(std::string_view category)
{
    auto loggerData = LoggerData::find();
    return loggerData
            ? loggerData->findCategory(category)
            : nullptr;
}

LogCategory& Logger::getCategory(size_t id)
{
    return LoggerData::get().getCategory(id);
}

void Logger::setRules(std::string rules)
{
    LoggerData::get().setRules(rules);
}

/******************************************************************************
 * LogCategory
 */
LogCategory::LogCategory(std::string_view name)
    : m_categoryName(name)
    , m_types(LogType::Fatal)
{
}

bool LogCategory::hasTypes(LogType types) const
{
    return (m_types & types) == types;
}

/******************************************************************************
 * LogCategoryRegistrar
 */
LogCategoryRegistrar::LogCategoryRegistrar(std::string_view name)
    : _name(name)
{
    category();
}

LogCategory* LogCategoryRegistrar::category()
{
    if (!_category)
    {
        _category = Logger::findCategory(_name);
    }
    return _category;
}

/******************************************************************************
 * LogLine
 */
namespace
{
std::string formatHeading(LogType type, const char *file, int line, const char *function)
{
    // strip path from file
    std::string sfile(file);
    sfile = sfile.substr(sfile.rfind('/') + 1);

    std::string ret = "[" + sfile + ":" + std::to_string(line) + " - " + function + "]";
    switch (type)
    {
        case LogType::Debug:
            break;
        case LogType::Warning:
            ret += " Warning:";
            break;
        case LogType::Info:
            ret += " Info:";
            break;
        case LogType::Fatal:
            ret += " FATAL:";
            break;
        default:
            break;
    }
    return ret;
}
}

LogLine::LogLine(LogType type, const char* file, unsigned line, const char* function)
    : LogLine(LoggerData::get().findCategory("default"), type, file, line, function)
{
}

LogLine::LogLine(LogCategory* category, LogType type, const char* file, unsigned line, const char* function)
    : m_category(category)
    , m_logType(type)
{
    if (isEnabled())
    {
        m_data << formatHeading(type, file, line, function);
    }
}

LogLine::~LogLine()
{
    if (isEnabled())
    {
        LoggerData::get().log(m_data.str());
    }
    if (m_logType == LogType::Fatal)
    {
        abort();
    }
}

LogLine& LogLine::operator<<(bool v)
{
    if (isEnabled())
    {
        m_data << (v ? " true " : " false");
    }
    return *this;
}

LogLine& LogLine::operator<<(char v)
{
    if (isEnabled())
    {
        m_data << ' ' << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(signed int v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::dec << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(unsigned int v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::dec << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(size_t v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::dec << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(const char* v)
{
    if (isEnabled())
    {
        m_data << ' ' << (v ? v : "(null)");
    }
    return *this;
}

LogLine& LogLine::operator<<(std::string_view v)
{
    if (isEnabled())
    {
        m_data << ' ' << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(const std::string& v)
{
    if (isEnabled())
    {
        m_data << ' ' << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(intptr_t v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::hex <<  v;
    }
    return *this;
}

LogLine& LogLine::operator<<(void* v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::hex << v;
    }
    return *this;
}

LogLine& LogLine::operator<<(double v)
{
    if (isEnabled())
    {
        m_data << ' ' << std::dec << v;
    }
    return *this;
}

}

#endif

