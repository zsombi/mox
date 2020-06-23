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

#include "event_dispatcher.h"

namespace mox
{

struct IdleBundle
{
    GMainContext* context = nullptr;
    IdleFunction idle;
    guint sourceId;

    explicit IdleBundle(GMainContext* context, IdleFunction&& idle)
        : context(context)
        , idle(std::forward<IdleFunction>(idle))
    {
        auto source = g_idle_source_new();
        g_source_set_callback(source, &IdleBundle::callback, gpointer(this), NULL);
        sourceId = g_source_attach(source, context);
        if (source->ref_count > 1)
        {
            g_source_unref(source);
        }
    }

    static gboolean callback(gpointer userData)
    {
        // sanity check, stop if the source is no longer part of the context!
        auto bundle = static_cast<IdleBundle*>(userData);
        if (!g_main_context_find_source_by_id(bundle->context, bundle->sourceId))
        {
            // The idle source is no longer in the context.
            delete bundle;
            return FALSE;
        }
        if (bundle->idle())
        {
            delete bundle;
            return FALSE;
        }
        // Need to reschedule again.
        return TRUE;
    }
};

/******************************************************************************
 * RunLoop idle handlers
 */
void GlibRunLoop::onIdleOverride(IdleFunction&& idle)
{
    new IdleBundle(context, std::forward<IdleFunction>(idle));
}

void GlibRunLoopHook::onIdleOverride(IdleFunction&& idle)
{
    new IdleBundle(context, std::forward<IdleFunction>(idle));
}

} // mox
