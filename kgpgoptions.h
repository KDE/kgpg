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

#include <KConfigDialog>

#include "ui_conf_gpg.h"
#include "ui_conf_ui2.h"
#include "ui_conf_servers.h"
#include "ui_conf_misc.h"
#include "ui_conf_decryption.h"

#include "kgpgkey.h"

class KFontChooser;
class KConfig;

class Encryption;

class Decryption : public QWidget, public Ui::Decryption
{
public:
  Decryption( QWidget *parent=0 ) : QWidget( parent) {
    setupUi(this);
  }
};

class MiscConf : public QWidget, public Ui::MiscConf
{
public:
  MiscConf( QWidget *parent=0 ) : QWidget( parent ) {
    setupUi( this );
  }
};


class UIConf : public QWidget, public Ui::UIConf
{
public:
  UIConf( QWidget *parent=0 ) : QWidget( parent ) {
    setupUi( this );
  }
};

class ServerConf : public QWidget, public Ui::ServerConf
{
public:
  ServerConf( QWidget *parent=0 ) : QWidget( parent ) {
    setupUi( this );
  }
};


class GPGConf : public QWidget, public Ui::GPGConf
{
public:
  GPGConf( QWidget *parent=0 ) : QWidget( parent ) {
    setupUi( this );
  }
};


class kgpgOptions : public KConfigDialog
{
    Q_OBJECT

public:
    explicit kgpgOptions(QWidget *parent = 0, const char *name = 0);
    ~kgpgOptions();

signals:
    void updateDisplay();
    void settingsUpdated();
    void changeFont(QFont);
    void homeChanged();
    void refreshTrust(KgpgCore::KgpgKeyTrust, QColor);

private slots:
    void slotChangeHome();
    void slotAddKeyServer();
	void slotChangeEncryptTo();
    void slotDelKeyServer();
    void slotEditKeyServer();
    void slotEditKeyServer(QListWidgetItem *);
    void slotEnableDeleteServer();
    void slotDefaultKeyServer();
    void updateWidgets();
    void updateWidgetsDefault();
    void updateSettings();
    void listKeys();
    void slotInstallDecrypt(const QString &mimetype);
    void slotInstallSign(const QString &mimetype);
    void slotRemoveMenu(const QString &menu);
    void slotSystrayEnable();

private:
    bool hasChanged();
    bool isDefault();
    bool isValidKeyserver(const QString &);

private:
    QStringList names;
    QStringList ids;
    QString alwaysKeyID;
    QString alwaysKeyName;
    QString fileEncryptionKey;
    QString gpgConfigPath;
    QString gpgBinPath;
    QString keyServer;              // Server stored in gnupg config
    QStringList serverList;         // Servers stored in kgpgrc
    QString defaultKeyServer;       // Default keyserver
    QStringList defaultServerList;  // Default list of servers including the default key server;
    QString defaultConfigPath;
    QString defaultHomePath;
    QString defaultBinPath;
    QPixmap pixkeySingle;
    QPixmap pixkeyDouble;
    QColor keyUltimate;
    QColor keyGood;
    QColor keyExpired;
    QColor keyMarginal;
    QColor keyBad;
    QColor keyUnknown;
    QColor keyRev;

    KFontChooser *m_fontchooser;
    KConfig *m_config;

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
    bool m_showsystray;
    int m_trayaction;
};

#endif // KGPGOPTIONS_H
