/***************************************************************************
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


#include "kgpgrevokewidget.h"

KgpgRevokeWidget::KgpgRevokeWidget( QWidget* parent )
    : QWidget( parent ), Ui_KgpgRevokeWidget()
{
    setupUi( this );
    connect(cbSave, SIGNAL(toggled(bool)), this, SLOT(cbSave_toggled(bool)));
}

void KgpgRevokeWidget::cbSave_toggled( bool isOn)
{
    kURLRequester1->setEnabled(isOn);
}
#include "kgpgrevokewidget.moc"
