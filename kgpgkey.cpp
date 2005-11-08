/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include "kgpgsettings.h"
#include "kgpgkey.h"

QString KgpgKey::algorithme(const int &v)
{
    if (v == 1)
        return QString("RSA");
    if ((v == 16) || (v == 20))
        return QString("ElGamal");
    if (v == 17)
        return QString("DSA");

    return QString("#" + v);
}

QString KgpgKey::trust(const QChar &c)
{
    if (c == 'o')
        return i18n("Unknown");
    if (c == 'i')
        return i18n("Invalid");
    if (c == 'd')
        return i18n("Disabled");
    if (c == 'r')
        return i18n("Revoked");
    if (c == 'e')
        return i18n("Expired");
    if (c == 'q')
        return i18n("Undefined");
    if (c == 'n')
        return i18n("None");
    if (c == 'm')
        return i18n("Marginal");
    if (c == 'f')
        return i18n("Full");
    if (c == 'u')
        return i18n("Ultimate");

    return QString();
}

QColor KgpgKey::color(const QChar &c)
{
    if (c == 'o')
        return KGpgSettings::colorUnknown();
    if (c == 'i')
        return KGpgSettings::colorBad();
    if (c == 'd')
        return KGpgSettings::colorBad();
    if (c == 'r')
        return KGpgSettings::colorRev();
    if (c == 'e')
        return KGpgSettings::colorBad();
    if (c == 'q')
        return KGpgSettings::colorUnknown();
    if (c == 'n')
        return KGpgSettings::colorUnknown();
    if (c == 'm')
        return KGpgSettings::colorBad();
    if (c == 'f')
        return KGpgSettings::colorGood();
    if (c == 'u')
        return KGpgSettings::colorGood();

    return KGpgSettings::colorUnknown();
}

QString KgpgKey::ownerTrust(const QChar &c)
{
    return QString();
}

int KgpgKey::ownerTrustIndex(const QChar &c)
{
    if (c == 'n')
        return 1;
    if (c == 'm')
        return 2;
    if (c == 'f')
        return 3;
    if (c == 'u')
        return 4;
    return 0;
}
