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


#ifndef KGPGREVOKEWIDGET_H
#define KGPGREVOKEWIDGET_H

#include "ui_kgpgrevokewidget.h"


class KgpgRevokeWidget : public QWidget, public Ui_KgpgRevokeWidget
{
    Q_OBJECT

public:
    KgpgRevokeWidget( QWidget* parent = 0 );
    

public slots:
   virtual void cbSave_toggled( bool isOn );
};

#endif
