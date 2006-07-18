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
    static QPixmap singleImage();
    static QPixmap pairImage();
    static QPixmap groupImage();
    static QPixmap orphanImage();
    static QPixmap signatureImage();
    static QPixmap userIdImage();
    static QPixmap photoImage();
    static QPixmap revokeImage();

private:
    static QPixmap m_keysingle;
    static QPixmap m_keypair;
    static QPixmap m_keygroup;
    static QPixmap m_keyoprpan;
    static QPixmap m_signature;
    static QPixmap m_userid;
    static QPixmap m_userphoto;
    static QPixmap m_revoke;
};

#endif // CORE_H
