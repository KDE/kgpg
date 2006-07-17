/***************************************************************************
                          kgpgcore.h  -  description
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

#ifndef CORE_H
#define CORE_H

#include <QPixmap>

class Core
{
public:
    static QPixmap getKeySingleImage();
    static QPixmap getKeyPairImage();
    static QPixmap getKeyGroupImage();

private:
    static QPixmap m_keysingle;
    static QPixmap m_keypair;
    static QPixmap m_keygroup;
};

#endif // CORE_H
