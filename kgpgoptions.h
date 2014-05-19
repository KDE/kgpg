/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2006,2007,2008,2009,2010,2011,2012,2013,2014
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 */
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

#include "ui_conf_gpg.h"
#include "ui_conf_ui2.h"
#include "ui_conf_servers.h"
#include "ui_conf_misc.h"
#include "ui_conf_decryption.h"

#include "core/kgpgkey.h"

#include <QPixmap>
#include <QStringListModel>
#include <QString>

#include <KConfigDialog>

class KFontChooser;
class KConfig;

class Encryption;
class GpgServerModel;
class KGpgItemModel;
class KeyListProxyModel;

class Decryption : public QWidget, public Ui::Decryption
{
public:
  explicit Decryption( QWidget *parent = 0 )
    : QWidget( parent)
  {
    setupUi(this);
  }
};

class MiscConf : public QWidget, public Ui::MiscConf
{
public:
  explicit MiscConf( QWidget *parent = 0 )
    : QWidget( parent )
  {
    setupUi( this );
  }
};


class UIConf : public QWidget, public Ui::UIConf
{
public:
  explicit UIConf( QWidget *parent = 0 )
    : QWidget( parent )
  {
    setupUi( this );
  }
};

class ServerConf : public QWidget, public Ui::ServerConf
{
public:
  explicit ServerConf( QWidget *parent = 0 )
    : QWidget( parent )
  {
    setupUi( this );
  }
};


class GPGConf : public QWidget, public Ui::GPGConf
{
public:
  explicit GPGConf( QWidget *parent = 0 )
    : QWidget( parent )
  {
    setupUi( this );
  }
};


class kgpgOptions : public KConfigDialog
{
    Q_OBJECT

public:
    explicit kgpgOptions(QWidget *parent = 0, KGpgItemModel *model = 0);
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
    void slotEditKeyServer(const QModelIndex &index);
    void slotChangeKeyServerButtonEnable();
    void slotDefaultKeyServer();
    void updateWidgets();
    void updateWidgetsDefault();
    void updateSettings();
    void listKeys();
    void slotInstallDecrypt(const QString &mimetype);
    void slotInstallSign(const QString &mimetype);
    void slotRemoveMenu(const QString &menu);
    void slotSystrayEnable();

protected:
    virtual bool hasChanged();
    virtual bool isDefault();

private:
    QStringList names;
    QStringList ids;
    QString alwaysKeyID;
    QString fileEncryptionKey;
    QString gpgConfigPath;
    QString keyServer;                    ///< Server stored in GnuPG config
    QStringList serverList;               ///< Servers stored in kgpgrc
    QString defaultKeyServer;             ///< Default keyserver
    QStringList defaultServerList;        ///< Default list of servers including the default key server;
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

    KConfig *m_config;

    Encryption * const m_page1;
    Decryption * const m_page2;
    UIConf * const m_page3;
    GPGConf * const m_page4;
    ServerConf * const m_page6;
    MiscConf * const m_page7;

    GpgServerModel * const m_serverModel; ///< model holding the servers
    KFontChooser * const m_fontchooser;

    bool m_useagent;
    bool m_defaultuseagent;
    bool m_encrypttoalways;
    bool m_defaultencrypttoalways;
    bool m_showsystray;
    int m_trayaction;
    int m_mailUats;
    int m_emailSortingIndex;
    QString m_emailTemplate;

    KGpgItemModel * const m_model;
    KeyListProxyModel * const m_combomodel;

    bool isValidKeyserver(const QString &);
};

#endif // KGPGOPTIONS_H
