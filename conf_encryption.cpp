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
    connect(kcfg_EncryptFilesTo, &QCheckBox::toggled, this, &Encryption::encrypt_files_to_toggled);
    connect(kcfg_AllowCustomEncryptionOptions, &QCheckBox::toggled, this, &Encryption::allow_custom_option_toggled);
    connect(encrypt_to_always, &QCheckBox::toggled, this, &Encryption::encrypt_to_always_toggled);
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
