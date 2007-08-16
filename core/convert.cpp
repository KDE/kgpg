/***************************************************************************
 *   Copyright (C) 2006 by Jimmy Gilles                                    *
 *   jimmygilles@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "convert.h"

#include <KGlobal>
#include <KLocale>

#include "kgpgsettings.h"

namespace KgpgCore
{

QString Convert::toString(const KgpgKeyAlgo &algorithme)
{
    if (algorithme == ALGO_UNKNOWN)
        return i18n("Unknown");
    if (algorithme == ALGO_RSA)
        return QString("RSA");
    if (algorithme == ALGO_DSA)
        return QString("DSA");
    if (algorithme == ALGO_ELGAMAL)
        return QString("ElGamal");
    if (algorithme == ALGO_DSA_ELGAMAL)
        return QString("DSA - ElGamal");

    return i18n("Unknown");
}

QString Convert::toString(const KgpgKeyOwnerTrust &ownertrust)
{
    if (ownertrust == OWTRUST_UNKNOWN)
        return i18n("Unknown");
    if (ownertrust == OWTRUST_UNDEFINED)
        return i18n("Do not Know");
    if (ownertrust == OWTRUST_NONE)
        return i18n("Do NOT Trust");
    if (ownertrust == OWTRUST_MARGINAL)
        return i18n("Marginally");
    if (ownertrust == OWTRUST_FULL)
        return i18n("Fully");
    if (ownertrust == OWTRUST_ULTIMATE)
        return i18n("Ultimately");

    return i18n("Unknown");
}

QString Convert::toString(const KgpgKeyTrust &trust)
{
    if (trust == TRUST_UNKNOWN)
        return i18n("Unknown");
    if (trust == TRUST_INVALID)
        return i18n("Invalid");
    if (trust == TRUST_DISABLED)
        return i18n("Disabled");
    if (trust == TRUST_REVOKED)
        return i18n("Revoked");
    if (trust == TRUST_EXPIRED)
        return i18n("Expired");
    if (trust == TRUST_UNDEFINED)
        return i18n("Undefined");
    if (trust == TRUST_NONE)
        return i18n("None");
    if (trust == TRUST_MARGINAL)
        return i18n("Marginal");
    if (trust == TRUST_FULL)
        return i18n("Full");
    if (trust == TRUST_ULTIMATE)
        return i18n("Ultimate");

    return i18n("Unknown");
}

QColor Convert::toColor(const KgpgKeyTrust &trust)
{
    if (trust == TRUST_UNKNOWN)
        return KGpgSettings::colorUnknown();
    if (trust == TRUST_INVALID)
        return KGpgSettings::colorBad();
    if (trust == TRUST_DISABLED)
        return KGpgSettings::colorBad();
    if (trust == TRUST_REVOKED)
        return KGpgSettings::colorRev();
    if (trust == TRUST_EXPIRED)
        return KGpgSettings::colorBad();
    if (trust == TRUST_UNDEFINED)
        return KGpgSettings::colorUnknown();
    if (trust == TRUST_NONE)
        return KGpgSettings::colorUnknown();
    if (trust == TRUST_MARGINAL)
        return KGpgSettings::colorBad();
    if (trust == TRUST_FULL)
        return KGpgSettings::colorGood();
    if (trust == TRUST_ULTIMATE)
        return KGpgSettings::colorGood();

    return KGpgSettings::colorUnknown();
}

QString Convert::toString(const QDate &date)
{
    return KGlobal::locale()->formatDate(date, KLocale::ShortDate);
}

} // namespace KgpgCore
