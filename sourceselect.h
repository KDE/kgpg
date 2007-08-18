/***************************************************************************
                          sourceselect.h  -  description
                             -------------------
    begin                : Mo April 30 2007
    copyright          : (C) 2007 by Lukas Kropatschek
    email                : lukas.krop@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SOURCESELECT_H
#define SOURCESELECT_H

#include "ui_sourceselect.h"

class SrcSelect : public QWidget, public Ui::SrcSelect
{
    Q_OBJECT

public:
    SrcSelect(QWidget *parent = 0);

public slots:
    void checkFile_toggled(bool);
};

#endif  // SOURCESELECT_H
