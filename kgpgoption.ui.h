/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/
#include <kcombobox.h>
#include <kurlrequester.h>

void KOptions::file_2_2_toggled( bool ison)
{
filekey_2_2->setEnabled(ison);
}


void KOptions::defaut_2_2_toggled( bool ison)
{
defautkey_2_2->setEnabled(ison);
}


void KOptions::custom_2_2_toggled( bool ison)
{
kLEcustom->setEnabled(ison);
}
