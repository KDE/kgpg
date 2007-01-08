/***************************************************************************
                          kgpgcore.cpp  -  description
                             -------------------
    begin                : Mon Jul 17 2006
    copyright            : (C) 2006 by Jimmy Gilles
    email                : jimmygilles@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kiconloader.h>

#include "images.h"

namespace KgpgCore
{

QPixmap Images::single()
{
    static QPixmap m_keysingle;
    if (m_keysingle.isNull())
        m_keysingle = KIconLoader::global()->loadIcon("kgpg_key1", K3Icon::Small, 20);
    return m_keysingle;
}

QPixmap Images::pair()
{
    static QPixmap m_keypair;
    if (m_keypair.isNull())
        m_keypair = KIconLoader::global()->loadIcon("kgpg_key2", K3Icon::Small, 20);
    return m_keypair;
}

QPixmap Images::group()
{
    static QPixmap m_keygroup;
    if (m_keygroup.isNull())
        m_keygroup = KIconLoader::global()->loadIcon("kgpg_key3", K3Icon::Small, 20);
    return m_keygroup;
}

QPixmap Images::orphan()
{
    static QPixmap m_keyoprpan;
    if (m_keyoprpan.isNull())
        m_keyoprpan = KIconLoader::global()->loadIcon("kgpg_key4", K3Icon::Small, 20);
    return m_keyoprpan;
}

QPixmap Images::signature()
{
    static QPixmap m_signature;
    if (m_signature.isNull())
        m_signature = KIconLoader::global()->loadIcon("signature", K3Icon::Small, 20);
    return m_signature;
}

QPixmap Images::userId()
{
    static QPixmap m_userid;
    if (m_userid.isNull())
        m_userid = KIconLoader::global()->loadIcon("kgpg_identity", K3Icon::Small, 20);
    return m_userid;
}

QPixmap Images::photo()
{
    static QPixmap m_userphoto;
    if (m_userphoto.isNull())
        m_userphoto = KIconLoader::global()->loadIcon("kgpg_photo", K3Icon::Small, 20);
    return m_userphoto;
}

QPixmap Images::revoke()
{
    static QPixmap m_revoke;
    if (m_revoke.isNull())
        m_revoke = KIconLoader::global()->loadIcon("stop", K3Icon::Small, 20);
    return m_revoke;
}

} // namespace KgpgCore
