/*

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>
                     Stefan Schimanski <1Stein@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/



#ifndef __NSPluginClassIface_h__
#define __NSPluginClassIface_h__


#include <qstringlist.h>
#include <qcstring.h>
#include <dcopobject.h>
#include <dcopref.h>


class NSPluginViewerIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:

  virtual void Shutdown() = 0;
  virtual DCOPRef NewClass(QString plugin) = 0;
};


class NSPluginClassIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:

  virtual DCOPRef NewInstance(QString mimeType, int mode, QStringList argn, QStringList argv) = 0;
  virtual QString GetMIMEDescription() = 0;

};


class NSPluginInstanceIface : virtual public DCOPObject
{
  K_DCOP

k_dcop:

  virtual void destroyPlugin() = 0;

  virtual int winId() = 0;

  virtual int setWindow(int remove=0) = 0;

  virtual void resizePlugin(int w, int h) = 0;

  virtual void setCallback(QCString _app, QCString _obj) = 0;

};


#endif

