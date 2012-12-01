/***************************************************************************
                             -------------------
    begin                : Mon Jul 8 2002
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

#ifndef CONF_ENCRYPTION_H
#define CONF_ENCRYPTION_H

#include "ui_conf_encryption.h"


class Encryption : public QWidget, public Ui_Encryption
{
    Q_OBJECT

public:
    explicit Encryption( QWidget* parent = 0 );
    
public slots:
    virtual void encrypt_to_always_toggled( bool );
    virtual void encrypt_files_to_toggled( bool );
    virtual void allow_custom_option_toggled( bool );

private:
   
};

#endif
