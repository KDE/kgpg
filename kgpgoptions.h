/***************************************************************************
                          kgpgoptions.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
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
#ifndef KGPGOPTIONS_H
#define KGPGOPTIONS_H


#include <kautoconfigdialog.h>

#include "conf_encryption.h"
#include "conf_decryption.h"
#include "conf_ui.h"
#include "conf_gpg.h"


class KConfig;

class kgpgOptions : public KAutoConfigDialog
{
        Q_OBJECT
public:
        kgpgOptions(QWidget *parent=0, const char *name=0);
        ~kgpgOptions();
        QStringList names,ids;
        Encryption *page1;
        Decryption *page2;
        UI *page3;
        GPGConf *page4;
        
private:
        KConfig *config;
        QString alwaysKeyID,alwaysKeyName;
        bool firstDisplay;

        QString fileEncryptionKey;
        QString gpgConfigPath;
        QString keyServer;
        QString defaultKeyServer;
        bool useAgent;
        bool defaultUseAgent;
        bool encryptToAlways;
        bool defaultEncryptToAlways;

private:
        bool hasChanged();
        bool isDefault();

private slots:
        void updateWidgets();
        void updateWidgetsDefault();
        void updateSettings();

        void listkey();
        QString namecode(QString kid);
        QString idcode(QString kname);
        void slotInstallDecrypt(QString mimetype);
        void slotInstallSign(QString mimetype);
        void slotRemoveMenu(QString menu);
signals:
        void updateDisplay();
        void settingsUpdated();
};

#endif
