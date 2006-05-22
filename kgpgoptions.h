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

#define GoodColor 0
#define BadColor 1
#define UnknownColor 2
#define RevColor 3

class KConfig;
class Encryption;
class Decryption;
class UIConf;
class GPGConf;
class ServerConf;
class ColorsConf;
class MiscConf;
class KFontChooser;
class KConfig;
class KSimpleConfig;

class kgpgOptions : public KConfigDialog
{
        Q_OBJECT
public:
        kgpgOptions(QWidget *parent=0, const char *name=0);
        ~kgpgOptions();
        QStringList names,ids;
        Encryption *page1;
        Decryption *page2;
        UIConf *page3;
        GPGConf *page4;
	ServerConf *page6;
	MiscConf *page7;
	KFontChooser *kfc;
        
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
        bool useAgent;
        bool defaultUseAgent;
        bool encryptToAlways;
        bool defaultEncryptToAlways;
	QStringList serverList;
	QString defaultConfigPath,defaultHomePath;
	QColor keyGood,keyBad,keyUnknown,keyRev;

private:
        bool hasChanged();
        bool isDefault();

private slots:
	void checkAdditionalState(bool);
	void slotAddKeyServer();
	void slotDelKeyServer();
	void slotDefaultKeyServer();
        void updateWidgets();
        void updateWidgetsDefault();
        void updateSettings();
	void insertAlwaysKey();
	void insertFileKey();

        void listkey();
        QString namecode(QString kid);
        QString idcode(QString kname);
        void slotInstallDecrypt(QString mimetype);
        void slotInstallSign(QString mimetype);
        void slotRemoveMenu(QString menu);
	void slotChangeHome();
signals:
        void updateDisplay();
        void settingsUpdated();
	void changeFont(QFont);
	void homeChanged();
	void refreshTrust(int, QColor);
	void installShredder();
	void reloadKeyList();
};

#endif // KGPGOPTIONS_H

