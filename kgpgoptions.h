/***************************************************************************
                          kgpgoptions.h  -  description
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
#ifndef KGPGOPTIONS_H
#define KGPGOPTIONS_H


#include <kconfigdialog.h>
#include <kinputdialog.h>
#include <klistview.h>
#include <kfontdialog.h>
#include <ksimpleconfig.h>

#include "conf_encryption.h"
#include "conf_decryption.h"
#include "conf_ui.h"
#include "conf_gpg.h"
#include "conf_servers.h"


class KConfig;

class kgpgOptions : public KConfigDialog
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
	ServerConf *page6;
	KFontChooser *page7;
        
private:
        KConfig *config;
        QString alwaysKeyID,alwaysKeyName;
        bool firstDisplay;
	
	QPixmap pixkeySingle,pixkeyDouble;
        QString fileEncryptionKey;
        QString gpgConfigPath;
        QString keyServer,defaultServerList;
        QString defaultKeyServer;
	QFont startFont;
	KSimpleConfig *ks;
        bool useAgent;
        bool defaultUseAgent;
        bool encryptToAlways;
        bool defaultEncryptToAlways;
	QStringList serverList;

private:
        bool hasChanged();
        bool isDefault();

private slots:
	void slotAddKeyServer();
	void slotDelKeyServer();
	void slotDefaultKeyServer();
        void updateWidgets();
        void updateWidgetsDefault();
        void updateSettings();

        void listkey();
        QString namecode(QString kid);
        QString idcode(QString kname);
        void slotInstallDecrypt(QString mimetype);
        void slotInstallSign(QString mimetype);
        void slotRemoveMenu(QString menu);
	void test();
signals:
        void updateDisplay();
        void settingsUpdated();
	void changeFont(QFont);
};

#endif
