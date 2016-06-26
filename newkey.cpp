/***************************************************************************
                          newkey.cpp  -  description
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

#include "newkey.h"

newKey::newKey(QWidget* parent)
      : QWidget(parent), Ui_newKey()
{
    setupUi(this);
    connect(CBsave, &QCheckBox::toggled, this, &newKey::CBsave_toggled);
}

void newKey::CBsave_toggled(bool isOn)
{
    kURLRequester1->setEnabled(isOn);
}
