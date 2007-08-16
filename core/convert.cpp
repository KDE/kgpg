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

QString Convert::toString(const KgpgKeyAlgo &algorithm)
{
	switch (algorithm)
    {
        case ALGO_RSA:          return QString("RSA");
        case ALGO_DSA:          return QString("DSA");
        case ALGO_ELGAMAL:      return QString("ElGamal");
        case ALGO_DSA_ELGAMAL:  return QString("DSA - ElGamal");
        case ALGO_UNKNOWN:
        default:                return i18n("Unknown");
	}
}

QString Convert::toString(const KgpgKeyOwnerTrust &ownertrust)
{
	switch (ownertrust)
    {
        case OWTRUST_UNDEFINED: return i18n("Do not Know");
        case OWTRUST_NONE:      return i18n("Do NOT Trust");
        case OWTRUST_MARGINAL:  return i18n("Marginally");
        case OWTRUST_FULL:      return i18n("Fully");
        case OWTRUST_ULTIMATE:  return i18n("Ultimately");
        case OWTRUST_UNKNOWN:
        default:                return i18n("Unknown");
	}
}

QString Convert::toString(const KgpgKeyTrust &trust)
{
	switch (trust)
    {
        case TRUST_INVALID:     return i18n("Invalid");
        case TRUST_DISABLED:    return i18n("Disabled");
        case TRUST_REVOKED:     return i18n("Revoked");
        case TRUST_EXPIRED:     return i18n("Expired");
        case TRUST_UNDEFINED:   return i18n("Undefined");
        case TRUST_NONE:        return i18n("None");
        case TRUST_MARGINAL:    return i18n("Marginal");
        case TRUST_FULL:        return i18n("Full");
        case TRUST_ULTIMATE:    return i18n("Ultimate");
        case TRUST_UNKNOWN:
        default:                return i18n("Unknown");
	}
}

QColor Convert::toColor(const KgpgKeyTrust &trust)
{
	switch (trust)
    {
        case TRUST_INVALID:
        case TRUST_DISABLED:
        case TRUST_EXPIRED:
        case TRUST_MARGINAL:    return KGpgSettings::colorBad();
        case TRUST_REVOKED:     return KGpgSettings::colorRev();
        case TRUST_UNDEFINED:
        case TRUST_NONE:        return KGpgSettings::colorUnknown();
        case TRUST_FULL:
        case TRUST_ULTIMATE:    return KGpgSettings::colorGood();
        case TRUST_UNKNOWN:
        default:                return KGpgSettings::colorUnknown();
	}
}

QString Convert::toString(const QDate &date)
{
    return KGlobal::locale()->formatDate(date, KLocale::ShortDate);
}

} // namespace KgpgCore
