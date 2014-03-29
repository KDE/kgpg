/*
 * Copyright (C) 2006 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2010,2013,2014 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */
/***************************************************************************
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

#ifndef CONVERT_H
#define CONVERT_H

#include "kgpgkey.h"

#include <gpgme.h>

class QString;
class QPixmap;

namespace KgpgCore
{

namespace Convert
{
    QString toString(const KgpgCore::KgpgKeyAlgo algorithm);
    QString toString(const gpgme_validity_t ownertrust);
    QString toString(const KgpgCore::KgpgKeyTrust trust);
    QString toString(const KgpgCore::KgpgSubKeyType type);
    KgpgKeyAlgo toAlgo(const uint v);
    KgpgKeyAlgo toAlgo(const QString &s);
    KgpgKeyTrust toTrust(const QChar &c);
    KgpgKeyTrust toTrust(const QString &s);
    gpgme_validity_t toOwnerTrust(const QChar &c);
    /**
     * @brief parse the GnuPG capabilities field
     * @param capString the capability string as returned by GnuPG
     * @param upper if the uppercase or lowercase version should be parsed
     */
    KgpgSubKeyType toSubType(const QString &capString, bool upper);
}

} // namespace KgpgCore

#endif // CONVERT_H
