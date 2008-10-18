/***************************************************************************
                          keyexport.cpp  -  description
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


#include "keyexport.h"

KeyExport::KeyExport(QWidget* parent, const QStringList& keyservers)
         : QWidget(parent), Ui_KeyExport()
{
    setupUi(this);

    if (keyservers.size() > 0)
    {
        checkServer->setEnabled(true); 
        destServer->addItems(keyservers);
    }
}

#include "keyexport.moc"
