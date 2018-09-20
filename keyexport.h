/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2011 Luis Ángel Fernández Fernández <laffdez@gmail.com>
 */

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

#include <QDialog>
#include <QStringList>

class KeyExport : public QDialog, public Ui_KeyExport
{
	Q_OBJECT

public:
    explicit KeyExport(QWidget *parent = nullptr, const QStringList &keyservers = QStringList());

protected:
        void accept() override;
};

#endif
