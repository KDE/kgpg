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

#ifndef IMAGES_H
#define IMAGES_H

#include <QPixmap>

namespace KgpgCore
{

class Images
{
public:
    static QPixmap single();
    static QPixmap pair();
    static QPixmap group();
    static QPixmap orphan();
    static QPixmap signature();
    static QPixmap userId();
    static QPixmap photo();
    static QPixmap revoke();

    /* Desktop image */
    static QPixmap kgpg();
};

} // namespace KgpgCore

#endif // IMAGES_H
