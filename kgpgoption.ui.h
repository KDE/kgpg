/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#include <kcombobox.h>
#include <klineedit.h>

void KgpgOptionDialog::custom_2_2_toggled( bool ison)
{
kLEcustom->setEnabled(ison);
}


void KgpgOptionDialog::file_2_2_toggled( bool ison)
{
kCBfilekey->setEnabled(ison);
}


void KgpgOptionDialog::defaut_2_2_toggled( bool ison)
{
kCBalwayskey->setEnabled(ison);
}
