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

#include "conf_encryption.h"

Encryption::Encryption( QWidget* parent )
    : QWidget( parent ), Ui_Encryption()
{
    setupUi( this );
    connect(kcfg_EncryptFilesTo, SIGNAL(toggled(bool)), this,SLOT(encrypt_files_to_toggled(bool)));
    connect(kcfg_AllowCustomEncryptionOptions, SIGNAL(toggled(bool)), this, SLOT(allow_custom_option_toggled(bool)));
    connect(encrypt_to_always, SIGNAL(toggled(bool)), this, SLOT(encrypt_to_always_toggled(bool))); 
}

void Encryption::encrypt_to_always_toggled(bool isOn)
{
    always_key->setEnabled(isOn);
}


void Encryption::encrypt_files_to_toggled(bool isOn)
{
    file_key->setEnabled(isOn);
}


void Encryption::allow_custom_option_toggled(bool isOn)
{
    kcfg_CustomEncryptionOptions->setEnabled(isOn);
}

#include "conf_encryption.moc"
