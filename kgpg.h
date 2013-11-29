/***************************************************************************
                          kgpg.h  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#ifndef KGPGAPPLET_H
#define KGPGAPPLET_H

#include <KUniqueApplication>
#include <KShortcut>
#include <KUrl>
#include <klocale.h>

class KCmdLineArgs;
class KeysManager;
class KGpgExternalActions;
class QString;

static const char * const EMailTemplateText=I18N_NOOP(
    "Hi,\n\nplease find attached the user id '%UIDNAME%' of your key %KEYID% signed by me. "
    "This mail is encrypted with that key to make sure you control both the email address and the key.\n\n"
    "If you have multiple user ids, I sent the signature for each user id separately to that user id's associated email address. "
    "You can import the signatures by running each through `gpg --import` after you have decrypted them with `gpg --decrypt`.\n\n"
    "If you are using KGpg store the attachment to disk and then import it. Just select `Import Key...` from `Keys` menu and open the file.\n\n"
    "Note that I did not upload your key to any keyservers. If you want this new signature to be available to others, please upload it yourself. "
    "With GnuPG this can be done using gpg --keyserver subkeys.pgp.net --send-key %KEYID%.\n\n" 
    "With KGpg you can right click on the key once you imported all user ids and choose `Export Public Key...`.\n\n"
    "If you have any questions, don't hesitate to ask.\n");

class KGpgApp : public KUniqueApplication
{
    Q_OBJECT

public:
    KGpgApp();
    ~KGpgApp();

    int newInstance ();
    bool running;
    KShortcut goHome;

private:
    KGpgExternalActions *w;
    KeysManager *s_keyManager;

private slots:
    void slotHandleQuit();
    void assistantOver(const QString &defaultKeyId);
};

#endif // KGPGAPPLET_H
