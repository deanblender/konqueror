/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <unistd.h>

#include <qlayout.h>

#include <kapp.h>
#include <dcopclient.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kdialog.h>

#include "rootopts.h"
#include "behaviour.h"
#include "fontopts.h"
#include "miscopts.h"
#include "trashopts.h"

#include "main.h"
#include "main.moc"


KonqyModule::KonqyModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  KConfig *config = new KConfig("konquerorrc", false, true);

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  QString groupName = "Icon Settings";
  behaviour = new KBehaviourOptions(config, groupName, true, this);
  tab->addTab(behaviour, i18n("&Behavior"));
  connect(behaviour, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KonqFontOptions(config, groupName, this);
  tab->addTab(font, i18n("&Appearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  trash = new KTrashOptions(config, "Trash", this);
  tab->addTab(trash, i18n("&Trash"));
  connect(trash, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  misc = new KMiscOptions(config, "Misc Defaults", this);
  tab->addTab(misc, i18n("&Other"));
  connect(misc, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));
}


void KonqyModule::load()
{
  behaviour->load();
  font->load();
  trash->load();
  misc->load();
}


void KonqyModule::save()
{
  behaviour->save();
  font->save();
  trash->save();
  misc->save();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonqMainViewIface", "reparseConfiguration()", data );
}


void KonqyModule::defaults()
{
  behaviour->defaults();
  font->defaults();
  trash->defaults();
  misc->defaults();
}


void KonqyModule::moduleChanged(bool state)
{
  emit changed(state);
}


KDesktopModule::KDesktopModule(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  KConfig *config = new KConfig("kdesktoprc", false, false);

  tab = new QTabWidget(this);

  root = new KRootOptions(config, this);
  tab->addTab(root, i18n("&Desktop"));
  connect(root, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  // Behaviour tab has nothing anymore for kdesktop - commented out
  // those use "Icon Settings" since they are read by KonqFMSettings
  //behaviour = new KBehaviourOptions(config, "Icon Settings", false, this);
  //tab->addTab(behaviour, i18n("&Behavior"));
  //connect(behaviour, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  font = new KonqFontOptions(config, "Icon Settings", this);
  tab->addTab(font, i18n("&Appearance"));
  connect(font, SIGNAL(changed(bool)), this, SLOT(moduleChanged(bool)));

  //should we add Trash also here ?
}


void KDesktopModule::load()
{
  //behaviour->load();
  root->load();
  font->load();
}


void KDesktopModule::save()
{
  //behaviour->save();
  root->save();
  font->save();

  // Tell kdesktop about the new config file
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "configure()", data );
}


void KDesktopModule::defaults()
{
  //behaviour->defaults();
  root->defaults();
  font->defaults();
}


void KDesktopModule::moduleChanged(bool state)
{
  emit changed(state);
}


void KDesktopModule::resizeEvent(QResizeEvent *)
{
  tab->setGeometry(0,0,width(),height());
}


extern "C"
{

  KCModule *create_konqueror(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KonqyModule(parent, name);
  }

  KCModule *create_desktop(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KDesktopModule(parent, name);
  }

}


