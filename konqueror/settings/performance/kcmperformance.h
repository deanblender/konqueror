/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _KCM_PERFORMANCE_H
#define _KCM_PERFORMANCE_H

#include <kcmodule.h>

namespace KCMPerformance
{

class Konqueror;
class SystemWidget;

class Config
    : public KCModule
    {
    Q_OBJECT
    public:
        Config( QWidget* parent_P, const char* name_P );
        virtual void load();
        virtual void save();
        virtual void defaults();
    private:
        Konqueror* konqueror_widget;
        SystemWidget* system_widget;
    };

class KonquerorConfig
    : public KCModule
    {
    Q_OBJECT
    public:
        KonquerorConfig( QWidget* parent_P, const char* name_P );
        virtual void load();
        virtual void save();
        virtual void defaults();
    private:
        Konqueror* widget;
    };

} // namespace

#endif
