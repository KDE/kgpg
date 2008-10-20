/***************************************************************************
                          kgpgoptions.cpp  -  description
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

#include "kgpgoptions.h"

// code for the option dialog box
#include <stdlib.h>

#include <QTextStream>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QFile>

#include <KStandardDirs>
#include <KInputDialog>
#include <KDesktopFile>
#include <KMessageBox>
#include <KFileDialog>
#include <KConfig>
#include <KLocale>
#include <KProcess>
#include <kdefakes.h>
#include <KFontChooser>

#include "images.h"
#include "kgpgsettings.h"

// #include "conf_decryption.h"
#include "conf_encryption.h"

using namespace KgpgCore;

//   main window
kgpgOptions::kgpgOptions(QWidget *parent, const char *name)
           : KConfigDialog(parent, name, KGpgSettings::self())
{
    m_config = new KConfig("kgpgrc", KConfig::SimpleConfig);
    
    // Initialize the default server and default additional servers.
    defaultKeyServer = "hkp://wwwkeys.pgp.net";
    defaultServerList << "hkp://search.keyserver.net" << "hkp://pgp.dtype.org";
    defaultServerList << "hkp://wwwkeys.us.pgp.net" << "hkp://subkeys.pgp.net";

    // Read the default keyserver from the gnupg settings.
    keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());
    
    // Read the aditional servers stored in kgpgrc
    KConfigGroup gr = m_config->group("Servers");
    serverList = gr.readEntry("additional_servers", defaultServerList);

    // Remove everything after a whitespace. This will normally be
    // ' (Default)' from KDE 3.x.x
    serverList.replaceInStrings(QRegExp(" .*"), "");

    defaultConfigPath = KUrl::fromPath(gpgConfigPath).fileName();
    defaultHomePath = KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash);
    defaultBinPath = KGpgSettings::gpgBinaryPath();

    m_showsystray = KGpgSettings::showSystray();

    kDebug(2100) << "Adding pages" ;
    m_page1 = new Encryption();
    m_page2 = new Decryption();
    m_page3 = new UIConf();
    m_page4 = new GPGConf();
    m_page6 = new ServerConf();
    m_page7 = new MiscConf();

    QVBoxLayout *fontlayout = new QVBoxLayout(m_page3->tabWidget3->widget(1));
    fontlayout->setSpacing(spacingHint());

    m_fontchooser = new KFontChooser(m_page3->tabWidget3->widget(1), false, QStringList(), false);
    m_fontchooser->setObjectName("kcfg_Font");
    fontlayout->addWidget(m_fontchooser);

    pixkeySingle = Images::single();
    pixkeyDouble = Images::pair();
    addPage(m_page1, i18n("Encryption"), "document-encrypt");
    addPage(m_page2, i18n("Decryption"), "document-decrypt");
    addPage(m_page3, i18n("Appearance"), "preferences-desktop-theme");
    addPage(m_page4, i18n("GnuPG Settings"), "kgpg");
    addPage(m_page6, i18n("Key Servers"), "network-wired");
    addPage(m_page7, i18n("Misc"), "preferences-other");

    // The following widgets are managed manually.
    connect(m_page4->changeHome, SIGNAL(clicked()), this, SLOT(slotChangeHome()));
    connect(m_page6->server_add, SIGNAL(clicked()), this, SLOT(slotAddKeyServer()));
    connect(m_page6->server_del, SIGNAL(clicked()), this, SLOT(slotDelKeyServer()));
    connect(m_page6->server_edit, SIGNAL(clicked()), this, SLOT(slotEditKeyServer()));
    connect(m_page6->server_default, SIGNAL(clicked()), this, SLOT(slotDefaultKeyServer()));
    connect(m_page6->ServerBox, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotEnableDeleteServer()));
    connect(m_page6->ServerBox, SIGNAL(executed(QListWidgetItem *)), this, SLOT(slotEditKeyServer(QListWidgetItem *)));

    keyUltimate = KGpgSettings::colorUltimate();
    keyGood = KGpgSettings::colorGood();
    keyMarginal = KGpgSettings::colorMarginal();
    keyExpired = KGpgSettings::colorExpired();
    keyUnknown = KGpgSettings::colorUnknown();
    keyRev = KGpgSettings::colorRev();
    keyBad = KGpgSettings::colorBad();
}

kgpgOptions::~kgpgOptions()
{
    delete m_config;
}

void kgpgOptions::slotChangeHome()
{
    QString gpgHome = KFileDialog::getExistingDirectory(m_page4->gpg_home_path->text(), this, i18n("New GnuPG Home Location"));
    if (gpgHome.isEmpty())
        return;

    if (!gpgHome.endsWith('/'))
        gpgHome.append('/');

    QString confPath = "options";
    if (!QFile(gpgHome + confPath).exists())
    {
        confPath = "gpg.conf";
        if (!QFile(gpgHome + confPath).exists())
        {
            if (KMessageBox::questionYesNo(this, i18n("No configuration file was found in the selected location.\nDo you want to create it now ?\n\nWithout configuration file, neither KGpg nor Gnupg will work properly."), i18n("No Configuration File Found"), KGuiItem(i18n("Create")), KGuiItem(i18n("Ignore"))) == KMessageBox::Yes) // Try to create config File by running gpg once
            {
                // start gnupg so that it will create a config file
                QString gpgbin = m_page4->gpg_bin_path->text();
                if (!QFile::exists(gpgbin))
                    gpgbin = "gpg";

                KProcess p;
                p << gpgbin << "--homedir" << gpgHome << "--no-tty" << "--list-secret-keys";
                p.execute();
                // end of creating config file

                confPath = "gpg.conf";
                QFile confFile(gpgHome + confPath);
                if (!confFile.open(QIODevice::WriteOnly))
                {
                    KMessageBox::sorry(this, i18n("Cannot create configuration file. Please check if destination media is mounted and if you have write access."));
                    return;
                }
                else
                {
                    QTextStream stream(&confFile);
                    stream << "#  Config file created by KGpg\n\n";
                    confFile.close();
                }
            }
            else
                confPath.clear();
        }
    }

    m_page4->gpg_conf_path->setText(confPath);
    m_page4->gpg_home_path->setText(gpgHome);
}

bool kgpgOptions::isValidKeyserver(const QString &server)
{
    if (server.isEmpty())
        return false;
    if (server.contains(' ')) 
    {
        KMessageBox::sorry(this, i18n("Key server URLs may not contain whitespace."));
        return false;
    }
    if (serverList.contains(server))
    {
        KMessageBox::sorry(this, i18n("Key server already in the list."));
        return false;
    }
        
    return true;
}

void kgpgOptions::slotAddKeyServer()
{
    QString newServer = KInputDialog::getText(i18n("Add New Key Server"), i18n("Server URL:"));

    if (!isValidKeyserver(newServer))
        return;

    serverList << newServer;
    QListWidgetItem *item = new QListWidgetItem(newServer, m_page6->ServerBox);
    m_page6->ServerBox->setCurrentItem(item);
    
    enableButtonApply(true);
}

void kgpgOptions::slotDelKeyServer()
{
  QListWidgetItem *cur = m_page6->ServerBox->takeItem(m_page6->ServerBox->currentRow());
  
  bool defaultDeleted = false;
  if (cur->text().contains( ' ' ) )
    defaultDeleted = true;
  
  // This doesn't seem to work -> Bug in QListWidget or KListWidget maybe?
  //m_page6->ServerBox->removeItemWidget(cur);
  
  cur = m_page6->ServerBox->currentItem();
  if (cur == NULL)
  {
    // The list is empty so disable the delete button.
    m_page6->server_del->setEnabled(false);
    return;
  }

  cur->setSelected(true);
  slotDefaultKeyServer();
  enableButtonApply(true);
}

void kgpgOptions::slotEditKeyServer()
{
    QListWidgetItem *cur = m_page6->ServerBox->currentItem();
    slotEditKeyServer(cur);
}

void kgpgOptions::slotEditKeyServer(QListWidgetItem *cur)
{
    if (cur == NULL)
        return;
  
    QString oldServer = cur->text();
    bool isDefault = false;
    if (oldServer.contains(' ')) {
        isDefault = true;
        oldServer = oldServer.section(" ", 0, 0);
    }

    QString newServer = KInputDialog::getText(i18n("Edit Key Server"), i18n("Server URL:"), oldServer).simplified();
    if (!isValidKeyserver(newServer))
        return;
    if (isDefault)
        newServer = i18nc("Mark default keyserver in GUI", "%1 (Default)", newServer);
    cur->setText(newServer);

    enableButtonApply(true);
}

void kgpgOptions::slotDefaultKeyServer()
{
    QListWidgetItem *curr = m_page6->ServerBox->currentItem();
    if (!curr->text().contains(' '))
    {
        // The current item is not already the default one so a couple of things
        // must be changed now:
        // 1. The "(Default)" mark must be removed in the GUI list.
        // 2. The current default server must be added to the additional server list.
        // 3. keyServer must be updated to the new default server.
        // 4. The new keyserver must be removed from the serverList.
        // 5. The new default keyServer must have the "(Default)" mark in the GUI.
        if (m_page6->ServerBox->findItems(keyServer, Qt::MatchContains).size() > 0)
        {
            QListWidgetItem *prev = m_page6->ServerBox->findItems(keyServer, Qt::MatchContains).first();
            prev->setText(prev->text().remove(' ' + i18nc("Mark default keyserver in GUI", "(Default)"))); // 1
            serverList << prev->text();     // 2
        }
        
        keyServer = curr->text();           // 3
        serverList.removeAll(curr->text()); // 4
        curr->setText(i18nc("Mark default keyserver in GUI", "%1 (Default)", curr->text())); // 5
        
        enableButtonApply(true);
    }
}

void kgpgOptions::slotEnableDeleteServer()
{
  if (!m_page6->server_del->isEnabled())
    m_page6->server_del->setEnabled(true);
}

void kgpgOptions::updateWidgets()
{
    alwaysKeyID = KgpgInterface::getGpgSetting("encrypt-to", KGpgSettings::gpgConfigPath());

    m_encrypttoalways = !alwaysKeyID.isEmpty();
    m_defaultencrypttoalways = false;

    m_page1->encrypt_to_always->setChecked(m_encrypttoalways);

    listKeys();
    fileEncryptionKey = KGpgSettings::fileEncryptionKey();
    if (!fileEncryptionKey.isEmpty())
        m_page1->file_key->setCurrentItem(fileEncryptionKey);

    if (!alwaysKeyName.isEmpty())
        m_page1->always_key->setCurrentItem(alwaysKeyName);

    gpgConfigPath = KGpgSettings::gpgConfigPath();
    m_page4->gpg_conf_path->setText(KUrl::fromPath(gpgConfigPath).fileName());
    m_page4->gpg_home_path->setText(KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash));
    gpgBinPath = KGpgSettings::gpgBinaryPath();
    m_page4->gpg_bin_path->setText(gpgBinPath);

    m_useagent = KgpgInterface::getGpgBoolSetting("use-agent", KGpgSettings::gpgConfigPath());
    m_defaultuseagent = false;

    m_page4->use_agent->setChecked(m_useagent);

    keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());

    if (keyServer.isEmpty())
    {
        keyServer = defaultKeyServer;
        serverList.removeAll(defaultKeyServer);
    }
    else if (keyServer != defaultKeyServer && !serverList.contains(defaultKeyServer))
        serverList << defaultKeyServer;

    m_page6->ServerBox->clear();
    m_page6->ServerBox->addItem(keyServer + ' ' + i18nc("Mark default keyserver in GUI", "(Default)"));
    m_page6->ServerBox->addItems(serverList);

    kDebug(2100) << "Finishing options" ;
}

void kgpgOptions::updateWidgetsDefault()
{
    m_page1->encrypt_to_always->setChecked(m_defaultencrypttoalways);
    m_page4->use_agent->setChecked(m_defaultuseagent);

    m_page4->gpg_conf_path->setText(defaultConfigPath);
    m_page4->gpg_home_path->setText(defaultHomePath);
    m_page4->gpg_bin_path->setText(defaultBinPath);

    m_page6->ServerBox->clear();
    m_page6->ServerBox->addItem(defaultKeyServer 
      + ' ' + i18nc("Mark default keyserver in GUI", "(Default)"));
    m_page6->ServerBox->addItems(defaultServerList);

    kDebug(2100) << "Finishing default options" ;
}

void kgpgOptions::updateSettings()
{
    // Update config path first!
    KGpgSettings::setGpgConfigPath(m_page4->gpg_home_path->text() + m_page4->gpg_conf_path->text());
    if (m_page4->gpg_home_path->text() != KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash))
    {
        if (m_page4->gpg_home_path->text() != defaultHomePath)
            setenv("GNUPGHOME", m_page4->gpg_home_path->text().toAscii(), 1);
        else
            setenv("GNUPGHOME", "", 1);
        emit homeChanged();

        gpgConfigPath = KGpgSettings::gpgConfigPath();
    }
    KGpgSettings::setGpgBinaryPath(m_page4->gpg_bin_path->text());
    gpgBinPath = KGpgSettings::gpgBinaryPath();

    // save selected keys for file encryption & always encrypt with
    if (m_page1->kcfg_EncryptFilesTo->isChecked())
        fileEncryptionKey = m_page1->file_key->currentText();
    else
        fileEncryptionKey.clear();

    if (fileEncryptionKey != KGpgSettings::fileEncryptionKey())
        KGpgSettings::setFileEncryptionKey(fileEncryptionKey);

    m_encrypttoalways = m_page1->encrypt_to_always->isChecked();

    if (m_encrypttoalways)
        alwaysKeyID = m_page1->always_key->currentText().section(':', 0, 0);
    else
        alwaysKeyID.clear();

    KgpgInterface::setGpgSetting("encrypt-to", alwaysKeyID, KGpgSettings::gpgConfigPath());

    emit changeFont(m_fontchooser->font());

    // install service menus
    if (m_page7->kcfg_SignMenu->currentIndex() == KGpgSettings::EnumSignMenu::AllFiles)
        slotInstallSign("application/octet-stream");
    else
        slotRemoveMenu("signfile.desktop");

    if (m_page7->kcfg_DecryptMenu->currentIndex() == KGpgSettings::EnumDecryptMenu::AllFiles)
        slotInstallDecrypt("application/octet-stream");
    else
    if (m_page7->kcfg_DecryptMenu->currentIndex() == KGpgSettings::EnumDecryptMenu::EncryptedFiles)
        slotInstallDecrypt("application/pgp-encrypted,application/pgp-signature,application/pgp-keys");
    else
        slotRemoveMenu("decryptfile.desktop");

    m_useagent = m_page4->use_agent->isChecked();

    if (m_useagent)
    {
        KgpgInterface::setGpgBoolSetting("use-agent", true, KGpgSettings::gpgConfigPath());
        KgpgInterface::setGpgBoolSetting("no-use-agent", false, KGpgSettings::gpgConfigPath());
    }
    else
    {
    //  KgpgInterface::setGpgBoolSetting("no-use-agent", true, KGpgSettings::gpgConfigPath());
        KgpgInterface::setGpgBoolSetting("use-agent", false, KGpgSettings::gpgConfigPath());
    }

    // Store the default server in ~/.gnupg
    KgpgInterface::setGpgSetting("keyserver", keyServer, KGpgSettings::gpgConfigPath());
    
    // Store additional servers in kgpgrc.
    serverList.clear();
    for (int i = 0; i < m_page6->ServerBox->count(); i++)
    {
        QString server = m_page6->ServerBox->item(i)->text();
        
        // Only store the additional servers in the config file.
        if (!server.contains(' '))
            serverList.append(server);
    }

    KConfigGroup gr = m_config->group("Servers");
    gr.writeEntry("additional_servers", serverList);

    if (keyUltimate != m_page3->kcfg_ColorUltimate->color())
        emit refreshTrust(TRUST_ULTIMATE, m_page3->kcfg_ColorUltimate->color());

    if (keyGood != m_page3->kcfg_ColorGood->color())
        emit refreshTrust(TRUST_FULL, m_page3->kcfg_ColorGood->color());

    if (keyExpired != m_page3->kcfg_ColorExpired->color())
        emit refreshTrust(TRUST_EXPIRED, m_page3->kcfg_ColorExpired->color());

    if (keyMarginal != m_page3->kcfg_ColorMarginal->color())
        emit refreshTrust(TRUST_MARGINAL, m_page3->kcfg_ColorMarginal->color());

    if (keyBad != m_page3->kcfg_ColorBad->color()) {
        emit refreshTrust(TRUST_INVALID, m_page3->kcfg_ColorBad->color());
        emit refreshTrust(TRUST_DISABLED, m_page3->kcfg_ColorBad->color());
    }

    if (keyUnknown != m_page3->kcfg_ColorUnknown->color()) {
        emit refreshTrust(TRUST_UNDEFINED, m_page3->kcfg_ColorUnknown->color());
        emit refreshTrust(TRUST_NONE, m_page3->kcfg_ColorUnknown->color());
        emit refreshTrust(TRUST_UNKNOWN, m_page3->kcfg_ColorUnknown->color());
    }

    if (keyRev != m_page3->kcfg_ColorRev->color())
        emit refreshTrust(TRUST_REVOKED, m_page3->kcfg_ColorRev->color());

    m_showsystray = m_page7->kcfg_ShowSystray->isChecked();
    KGpgSettings::setShowSystray(m_showsystray);

    KGpgSettings::self()->writeConfig();
    m_config->sync();
    
    emit settingsUpdated();
}

void kgpgOptions::listKeys()
{
	KgpgInterface *iface = new KgpgInterface;
	KgpgCore::KgpgKeyList keys = iface->readJoinedKeys(KgpgCore::TRUST_MARGINAL);
	delete iface;

	if (keys.size() == 0) {
		ids += QString("0");
		m_page1->file_key->addItem(i18nc("no key available", "none"));
		m_page1->always_key->addItem(i18nc("no key available", "none"));
	} else {
		for (int i = 0; i < keys.size(); i++) {
			KgpgCore::KgpgKey key = keys.at(i);

			names += key.name();
			ids += key.id();
			QString text = key.id() + ": " + key.name() + " <" + key.email() + '>';
			if (key.id() == alwaysKeyID)
				alwaysKeyName = text;

			if (key.secret()) {
				m_page1->file_key->addItem(pixkeyDouble, text);
				m_page1->always_key->addItem(pixkeyDouble, text);
			} else {
				m_page1->file_key->addItem(pixkeySingle, text);
				m_page1->always_key->addItem(pixkeySingle, text);
			}
		}
	}
}

void kgpgOptions::slotInstallDecrypt(const QString &mimetype)
{
    QString path = KStandardDirs::locateLocal("data", "konqueror/servicemenus/decryptfile.desktop");
    KDesktopFile configl2(path);
    if (configl2.isImmutable() == false)
    {
	KConfigGroup gr = configl2.group("Desktop Entry");

	gr.writeXdgListEntry("MimeType", QStringList() << mimetype);
	gr.writeEntry("X-KDE-ServiceTypes", "KonqPopupMenu/Plugin");
	gr.writeEntry("Actions", "decrypt");

	gr = configl2.group("Desktop Action decrypt");
	gr.writeEntry("Name", i18n("Decrypt File"));
	//gr.writeEntry("Icon", "decrypt_file");
	gr.writeEntry("Exec", "kgpg %U");
        //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
    }
}

void kgpgOptions::slotInstallSign(const QString &mimetype)
{
    QString path = KStandardDirs::locateLocal("services", "ServiceMenus/signfile.desktop");
    KDesktopFile configl2(path);
    if (configl2.isImmutable() ==false)
    {
	KConfigGroup gr = configl2.group("Desktop Entry");
	gr.writeXdgListEntry("MimeType", QStringList() << mimetype);
	gr.writeEntry("X-KDE-ServiceTypes", "KonqPopupMenu/Plugin");
	gr.writeEntry("Actions", "sign");

	gr = configl2.group("Desktop Action sign");
	gr.writeEntry("Name", i18n("Sign File"));
	//gr.writeEntry("Icon", "sign_file");
	gr.writeEntry("Exec","kgpg -S %F");
        //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
    }
}

void kgpgOptions::slotRemoveMenu(const QString &menu)
{
    QString path = KStandardDirs::locateLocal("services", "ServiceMenus/" + menu);
    QFile qfile(path);
    if (qfile.exists())
        qfile.remove();

    //if (!qfile.remove()) KMessageBox::sorry(this,i18n("Cannot remove service menu. Check permissions"));
    //else KMessageBox::information(this,i18n("Service menu 'Decrypt File' has been removed."));

    //else KMessageBox::sorry(this,i18n("No service menu found"));
}

bool kgpgOptions::hasChanged()
{
    if (m_page1->kcfg_EncryptFilesTo->isChecked() && (m_page1->file_key->currentText() != fileEncryptionKey))
        return true;

    if (m_page1->encrypt_to_always->isChecked() != m_encrypttoalways)
        return true;

    if (m_page1->encrypt_to_always->isChecked() && (m_page1->always_key->currentText().section(':', 0, 0) != alwaysKeyID))
        return true;

    if (m_page4->gpg_conf_path->text() != KUrl::fromPath(gpgConfigPath).fileName())
        return true;

    if (m_page4->gpg_home_path->text() != KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash))
        return true;

    if (m_page4->gpg_bin_path->text() != gpgBinPath)
        return true;

    if (m_page4->use_agent->isChecked() != m_useagent)
        return true;

    // Did the number of servers change?
    if (m_page6->ServerBox->count() != serverList.size() + 1)
      return true;
    
    // Did the actual value of the servers change than?
    for (int i = 0; i < m_page6->ServerBox->count(); i++)
    {
        QString server = m_page6->ServerBox->item(i)->text();
        
        if (server.contains(' '))
        {
            // This is the current server marked as default in the GUI.
            server.remove(' ' + i18nc( "Remove default marker from GUI if it is there", "(Default)"));
            if (keyServer != server)
                return true;
        }
        else if (!serverList.contains(server))
            return true;
    }
    
    if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
        return true;

    return false;
}

bool kgpgOptions::isDefault()
{
    if (m_page1->encrypt_to_always->isChecked() != m_defaultencrypttoalways)
        return false;

    if (m_page4->gpg_conf_path->text() != defaultConfigPath)
        return false;

    if (m_page4->gpg_home_path->text() != defaultHomePath)
        return false;

    if (m_page4->gpg_bin_path->text() != defaultBinPath)
        return false;

    if (m_page4->use_agent->isChecked() != m_defaultuseagent)
        return false;
    
    for (int i = 0; i < m_page6->ServerBox->count(); i++)
    {
        QString server = m_page6->ServerBox->item(i)->text();
        
        if (server.contains(' '))
        {
            // This is the current server marked as default in the GUI.
            server.remove(' ' + i18nc( "Remove default marker from GUI if it is there", "(Default)"));
            if (defaultKeyServer != server)
                return false;
        }
        else if (!defaultServerList.contains(server))
            return false;
    }

    if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
        return false;

    return true;
}

#include "kgpgoptions.moc"
