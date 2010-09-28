/***************************************************************************
                          kgpgsettings_addons.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright          : (C) 2003Waldo Bastian
    email                : bastian@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGSETTINGS_ADDONS_H
#define KGPGSETTINGS_ADDONS_H

public:
   static
   QString defaultKey()
   {
     if (self()->mDefaultKey.isEmpty())
     {
        self()->mDefaultKey = KgpgInterface::getGpgSetting(QLatin1String( "default-key" ), gpgConfigPath());
     }
     return self()->mDefaultKey;
   }

   static
   void setDefaultKey(const QString &_defaultKey)
   {
     self()->mDefaultKey = _defaultKey;
     KgpgInterface::setGpgSetting(QLatin1String( "default-key" ), _defaultKey, gpgConfigPath());
   }

private:
   QString mDefaultKey;

#endif //KGPGSETTINGS_ADDONS_H
