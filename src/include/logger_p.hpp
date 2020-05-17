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

#ifndef LOGGER_P_HPP
#define LOGGER_P_HPP

#if defined(MOX_ENABLE_LOGS)

#include <mox/utils/log/logger.hpp>

namespace mox
{

class LoggerData
{
    using LogCategoryCollection = std::vector<LogCategory>;

    std::mutex m_mutex;
    LoggerInterfacePtr m_logger;
    LogCategoryCollection m_categories;

    static inline LoggerData* g_logger = nullptr;

    LoggerData();
    ~LoggerData();

public:
    static LoggerData& get();
    static LoggerData* find();

    void setRules(std::string rules);
    void setRule(std::string rule);

    void log(LogCategory& category, LogType type, std::string_view heading, const std::string& text);

    void setLogger(LoggerInterfacePtr&& logger);

    size_t addCategory(LogCategory&& category);
    LogCategory* findCategory(std::string_view category);
    LogCategory& getCategory(size_t id);
};

}

#endif
#endif // LOGGER_P_HPP
