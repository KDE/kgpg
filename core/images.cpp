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

#include <KIconLoader>

namespace KgpgCore
{

namespace Images
{

QPixmap single()
{
    static QPixmap single;
    if (single.isNull())
        single = KIconLoader::global()->loadIcon(QLatin1String( "key-single" ), KIconLoader::Small, 20);
    return single;
}

QPixmap pair()
{
    static QPixmap pair;
    if (pair.isNull())
        pair = KIconLoader::global()->loadIcon(QLatin1String( "key-pair" ), KIconLoader::Small, 20);
    return pair;
}

QPixmap group()
{
    static QPixmap group;
    if (group.isNull())
        group = KIconLoader::global()->loadIcon(QLatin1String( "key-group" ), KIconLoader::Small, 20);
    return group;
}

QPixmap orphan()
{
    static QPixmap oprpan;
    if (oprpan.isNull())
        oprpan = KIconLoader::global()->loadIcon(QLatin1String( "key-orphan" ), KIconLoader::Small, 20);
    return oprpan;
}

QPixmap signature()
{
    static QPixmap signature;
    if (signature.isNull())
        signature = KIconLoader::global()->loadIcon(QLatin1String( "application-pgp-signature" ), KIconLoader::Small, 20);
    return signature;
}

QPixmap userId()
{
    static QPixmap userid;
    if (userid.isNull())
        userid = KIconLoader::global()->loadIcon(QLatin1String( "x-office-contact" ), KIconLoader::Small, 20);
    return userid;
}

QPixmap photo()
{
    static QPixmap photo;
    if (photo.isNull())
        photo = KIconLoader::global()->loadIcon(QLatin1String( "image-x-generic" ), KIconLoader::Small, 20);
    return photo;
}

QPixmap revoke()
{
    static QPixmap revoke;
    if (revoke.isNull())
        revoke = KIconLoader::global()->loadIcon(QLatin1String( "dialog-error" ), KIconLoader::Small, 20);
    return revoke;
}

QPixmap kgpg()
{
    static QPixmap kgpg;
    if (kgpg.isNull())
        kgpg = KIconLoader::global()->loadIcon(QLatin1String( "kgpg" ), KIconLoader::Desktop);
    return kgpg;
}

} // namespace Images

} // namespace KgpgCore
