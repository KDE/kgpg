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

public:
   static
   QString defaultKey()
   {
     if (self()->mDefaultKey.isEmpty())
     {
        self()->mDefaultKey = KgpgInterface::getGpgSetting("default-key", gpgConfigPath());
        if (!self()->mDefaultKey.isEmpty())
           self()->mDefaultKey.prepend("0x");
     }
     return self()->mDefaultKey;
   }
   
   static
   void setDefaultKey(const QString &_defaultKey)
   {
     self()->mDefaultKey = _defaultKey;
     KgpgInterface::setGpgSetting("default-key",_defaultKey.right(8),gpgConfigPath());
     if (!_defaultKey.startsWith("0x"))
        self()->mDefaultKey.prepend("0x");
   }
   
private:
   QString mDefaultKey;
