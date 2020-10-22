/*
 * Copyright (C) 2006 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
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

#include "images.h"


namespace KgpgCore
{

namespace Images
{

QIcon single()
{
    static QIcon single;
    if (single.isNull())
        single = QIcon::fromTheme(QStringLiteral("key-single"));
    return single;
}

QIcon pair()
{
    static QIcon pair;
    if (pair.isNull())
        pair = QIcon::fromTheme(QStringLiteral("key-pair"));
    return pair;
}

QIcon group()
{
    static QIcon group;
    if (group.isNull())
        group = QIcon::fromTheme(QStringLiteral("key-group"));
    return group;
}

QIcon orphan()
{
    static QIcon oprpan;
    if (oprpan.isNull())
        oprpan = QIcon::fromTheme(QStringLiteral("key-orphan"));
    return oprpan;
}

QIcon signature()
{
    static QIcon signature;
    if (signature.isNull())
        signature = QIcon::fromTheme(QStringLiteral("application-pgp-signature"));
    return signature;
}

QIcon userId()
{
    static QIcon userid;
    if (userid.isNull())
        userid = QIcon::fromTheme(QStringLiteral("x-office-contact"));
    return userid;
}

QIcon photo()
{
    static QIcon photo;
    if (photo.isNull())
        photo = QIcon::fromTheme(QStringLiteral("image-x-generic"));
    return photo;
}

QIcon revoke()
{
    static QIcon revoke;
    if (revoke.isNull())
        revoke = QIcon::fromTheme(QStringLiteral("dialog-error"));
    return revoke;
}

QIcon kgpg()
{
    static QIcon kgpg;
    if (kgpg.isNull())
        kgpg = QIcon::fromTheme(QStringLiteral("kgpg"));
    return kgpg;
}

} // namespace Images

} // namespace KgpgCore
