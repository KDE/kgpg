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

#include <QPixmap>

#include <kconfigdialog.h>

class KSimpleConfig;
class KFontChooser;
class KConfig;

class Encryption;
class Decryption;
class UIConf;
class GPGConf;
class ServerConf;
class ColorsConf;
class MiscConf;

class kgpgOptions : public KConfigDialog
{
    Q_OBJECT

public:
    enum KeyColors
    {
        GoodColor = 0,
        BadColor = 1,
        UnknownColor = 2,
        RevColor = 3
    };

    kgpgOptions(QWidget *parent = 0, const char *name = 0);
    ~kgpgOptions();

signals:
    void updateDisplay();
    void settingsUpdated();
    void changeFont(QFont);
    void homeChanged();
    void refreshTrust(int, QColor);
    void installShredder();

private slots:
    void slotChangeHome();
    void slotAddKeyServer();
    void slotDelKeyServer();
    void slotEditKeyServer();
    void slotDefaultKeyServer();
    void updateWidgets();
    void updateWidgetsDefault();
    void updateSettings();
    void listKeys();
    void slotInstallDecrypt(QString mimetype);
    void slotInstallSign(QString mimetype);
    void slotRemoveMenu(QString menu);

private:
    bool hasChanged();
    bool isDefault();

private:
    QStringList serverList;
    QStringList names;
    QStringList ids;
    QString alwaysKeyID;
    QString alwaysKeyName;
    QString fileEncryptionKey;
    QString gpgConfigPath;
    QString keyServer;
    QString defaultServerList;
    QString defaultKeyServer;
    QString defaultConfigPath;
    QString defaultHomePath;
    QPixmap pixkeySingle;
    QPixmap pixkeyDouble;
    QColor keyGood;
    QColor keyBad;
    QColor keyUnknown;
    QColor keyRev;

    KFontChooser *m_fontchooser;
    KSimpleConfig *m_config;

    Encryption *m_page1;
    Decryption *m_page2;
    UIConf *m_page3;
    GPGConf *m_page4;
    ServerConf *m_page6;
    MiscConf *m_page7;

    bool m_useagent;
    bool m_defaultuseagent;
    bool m_encrypttoalways;
    bool m_defaultencrypttoalways;
};

#endif // KGPGOPTIONS_H
