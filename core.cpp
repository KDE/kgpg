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

QPixmap Core::getKeySingleImage()
{
    if (m_keysingle.isNull())
        m_keysingle = KGlobal::iconLoader()->loadIcon("kgpg_key1", K3Icon::Small, 20);
    return m_keysingle;
}

QPixmap Core::getKeyPairImage()
{
    if (m_keypair.isNull())
        m_keypair = KGlobal::iconLoader()->loadIcon("kgpg_key2", K3Icon::Small, 20);
    return m_keypair;
}

QPixmap Core::getKeyGroupImage()
{
    if (m_keygroup.isNull())
        m_keygroup = KGlobal::iconLoader()->loadIcon("kgpg_key3", K3Icon::Small, 20);
    return m_keygroup;
}
