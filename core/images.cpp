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

#include <QIcon>
#include <KIconLoader>

namespace KgpgCore
{

namespace Images
{

QPixmap single()
{
    static QPixmap single;
    if (single.isNull())
        single = QIcon::fromTheme(QStringLiteral("key-single")).pixmap(IconSize(KIconLoader::Small));
    return single;
}

QPixmap pair()
{
    static QPixmap pair;
    if (pair.isNull())
        pair = QIcon::fromTheme(QStringLiteral("key-pair")).pixmap(IconSize(KIconLoader::Small));
    return pair;
}

QPixmap group()
{
    static QPixmap group;
    if (group.isNull())
        group = QIcon::fromTheme(QStringLiteral("key-group")).pixmap(IconSize(KIconLoader::Small));
    return group;
}

QPixmap orphan()
{
    static QPixmap oprpan;
    if (oprpan.isNull())
        oprpan = QIcon::fromTheme(QStringLiteral("key-orphan")).pixmap(IconSize(KIconLoader::Small));
    return oprpan;
}

QPixmap signature()
{
    static QPixmap signature;
    if (signature.isNull())
        signature = QIcon::fromTheme(QStringLiteral("application-pgp-signature")).pixmap(IconSize(KIconLoader::Small));
    return signature;
}

QPixmap userId()
{
    static QPixmap userid;
    if (userid.isNull())
        userid = QIcon::fromTheme(QStringLiteral("x-office-contact")).pixmap(IconSize(KIconLoader::Small));
    return userid;
}

QPixmap photo()
{
    static QPixmap photo;
    if (photo.isNull())
        photo = QIcon::fromTheme(QStringLiteral("image-x-generic")).pixmap(IconSize(KIconLoader::Small));
    return photo;
}

QPixmap revoke()
{
    static QPixmap revoke;
    if (revoke.isNull())
        revoke = QIcon::fromTheme(QStringLiteral("dialog-error")).pixmap(IconSize(KIconLoader::Small));
    return revoke;
}

QPixmap kgpg()
{
    static QPixmap kgpg;
    if (kgpg.isNull())
        kgpg = QIcon::fromTheme(QStringLiteral("kgpg")).pixmap(IconSize(KIconLoader::Desktop));
    return kgpg;
}

} // namespace Images

} // namespace KgpgCore
