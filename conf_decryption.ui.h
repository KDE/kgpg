/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void Decryption::use_agent_toggled( bool isOn)
{
int pos=custom_decrypt->text().find("--no-use-agent",0,FALSE);
if (isOn)
{
    if (pos==-1) custom_decrypt->setText(custom_decrypt->text().stripWhiteSpace()+" --no-use-agent");
}
else if (pos!=-1) custom_decrypt->setText(custom_decrypt->text().remove(pos,14));
}
