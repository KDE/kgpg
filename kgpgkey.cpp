/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include "kgpgkey.h"

QString KgpgKey::algorithme(const int &v)
{

}

QString KgpgKey::trust(const QChar &c)
{
    if (c == 'f')
        return i18n("Full");
    if (c == 'u')
        return i18n("Ultimate");

    return QString();
}

QString KgpgKey::ownerTrust(const QChar &c)
{

}
