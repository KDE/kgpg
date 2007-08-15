/***************************************************************************
                          keyexport.h  -  description
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


#ifndef KEYEXPORT_H
#define KEYEXPORT_H

#include "ui_keyexport.h"


class KeyExport : public QWidget, public Ui_KeyExport
{
    Q_OBJECT

public:
    KeyExport( QWidget* parent = 0, const QStringList *keyservers = 0 );

public slots:
   virtual void checkFile_toggled(bool);
};

#endif
