/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void Encryption::encrypt_to_always_toggled( bool isOn)
{
always_key->setEnabled(isOn);
}


void Encryption::encrypt_files_to_toggled( bool isOn)
{
file_key->setEnabled(isOn);
}


void Encryption::allow_custom_option_toggled( bool isOn)
{
kcfg_CustomEncryptionOptions->setEnabled(isOn);
}
