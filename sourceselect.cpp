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

#include "sourceselect.h"

SrcSelect::SrcSelect(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
	connect(checkFile, &QRadioButton::toggled, this, &SrcSelect::checkFile_toggled);
	connect(checkServer, &QRadioButton::toggled, this, &SrcSelect::checkServer_toggled);
}

void SrcSelect::checkFile_toggled(bool isOn)
{
	newFilename->setEnabled(isOn);
}

void SrcSelect::checkServer_toggled(bool isOn)
{
	keyIds->setEnabled(isOn);
}
