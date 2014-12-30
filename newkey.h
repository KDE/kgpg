/***************************************************************************
                          newkey.h  -  description
                             -------------------
    begin                : Thu Jul 4 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NEWKEY_H
#define NEWKEY_H

#include <kgpgcompiler.h>
#include "ui_newkey.h"

class newKey : public QWidget, public Ui_newKey
{
    Q_OBJECT

public:
    explicit newKey(QWidget* parent = Q_NULLPTR);


public slots:
   void CBsave_toggled(bool);
};

#endif
