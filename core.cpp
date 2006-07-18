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

#include "core.h"

QPixmap Core::m_keysingle;
QPixmap Core::m_keypair;
QPixmap Core::m_keygroup;
QPixmap Core::m_keyoprpan;
QPixmap Core::m_signature;
QPixmap Core::m_userid;
QPixmap Core::m_userphoto;
QPixmap Core::m_revoke;

QPixmap Core::singleImage()
{
    if (m_keysingle.isNull())
        m_keysingle = KGlobal::iconLoader()->loadIcon("kgpg_key1", K3Icon::Small, 20);
    return m_keysingle;
}

QPixmap Core::pairImage()
{
    if (m_keypair.isNull())
        m_keypair = KGlobal::iconLoader()->loadIcon("kgpg_key2", K3Icon::Small, 20);
    return m_keypair;
}

QPixmap Core::groupImage()
{
    if (m_keygroup.isNull())
        m_keygroup = KGlobal::iconLoader()->loadIcon("kgpg_key3", K3Icon::Small, 20);
    return m_keygroup;
}

QPixmap Core::orphanImage()
{
    if (m_keyoprpan.isNull())
        m_keyoprpan = KGlobal::iconLoader()->loadIcon("kgpg_key4", K3Icon::Small, 20);
    return m_keyoprpan;
}

QPixmap Core::signatureImage()
{
    if (m_signature.isNull())
        m_signature = KGlobal::iconLoader()->loadIcon("signature", K3Icon::Small, 20);
    return m_signature;
}

QPixmap Core::userIdImage()
{
    if (m_userid.isNull())
        m_userid = KGlobal::iconLoader()->loadIcon("kgpg_identity", K3Icon::Small, 20);
    return m_userid;
}

QPixmap Core::photoImage()
{
    if (m_userphoto.isNull())
        m_userphoto = KGlobal::iconLoader()->loadIcon("kgpg_photo", K3Icon::Small, 20);
    return m_userphoto;
}

QPixmap Core::revokeImage()
{
    if (m_revoke.isNull())
        m_revoke = KGlobal::iconLoader()->loadIcon("stop", K3Icon::Small, 20);
    return m_revoke;
}
