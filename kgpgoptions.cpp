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

#include "keylistproxymodel.h"
#include "kgpgitemmodel.h"
#include "kgpginterface.h"

// #include "conf_decryption.h"
#include "conf_encryption.h"

using namespace KgpgCore;

//   main window
kgpgOptions::kgpgOptions(QWidget *parent, KGpgItemModel *model)
           : KConfigDialog(parent, QLatin1String( "settings" ), KGpgSettings::self()),
	   m_config(new KConfig( QLatin1String( "kgpgrc" ), KConfig::SimpleConfig)),
	   m_page1(new Encryption()),
	   m_page2(new Decryption()),
	   m_page3(new UIConf()),
	   m_page4(new GPGConf()),
	   m_page6(new ServerConf()),
	   m_page7(new MiscConf()),
	   m_fontchooser(new KFontChooser(m_page3->tabWidget3->widget(1), KFontChooser::NoDisplayFlags, QStringList())),
	   m_model(model),
	   m_combomodel(new KeyListProxyModel(this, KeyListProxyModel::SingleColumnIdFirst))
{
	m_page7->EmailTemplateEdit->setPlainText(KGpgSettings::emailTemplate());

	m_combomodel->setKeyModel(m_model);
	m_combomodel->setTrustFilter(KgpgCore::TRUST_MARGINAL);
	m_combomodel->sort(0);

	// Initialize the default server and the default server list.
	defaultKeyServer = QLatin1String("hkp://wwwkeys.pgp.net");
	defaultServerList << defaultKeyServer
			<< QLatin1String("hkp://search.keyserver.net")
			<< QLatin1String("hkp://pgp.dtype.org")
			<< QLatin1String("hkp://subkeys.pgp.net");

	// Read the default keyserver from the GnuPG settings.
	keyServer = KgpgInterface::getGpgSetting(QLatin1String( "keyserver" ), KGpgSettings::gpgConfigPath());

	// Read the servers stored in kgpgrc
	serverList = KGpgSettings::keyServers();

	// Remove everything after a whitespace. This will normally be
	// ' (Default)' from KDE 3.x.x
	serverList.replaceInStrings(QRegExp( QLatin1String( " .*") ), QLatin1String( "" ) );

	defaultConfigPath = KUrl::fromPath(gpgConfigPath).fileName();
	defaultHomePath = KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash);
	defaultBinPath = KGpgSettings::gpgBinaryPath();

	m_showsystray = KGpgSettings::showSystray();
	m_trayaction = KGpgSettings::leftClick();
	m_mailUats = KGpgSettings::mailUats();

	QVBoxLayout *fontlayout = new QVBoxLayout(m_page3->tabWidget3->widget(1));
	fontlayout->setSpacing(spacingHint());

	m_fontchooser->setObjectName( QLatin1String("kcfg_Font" ));
	fontlayout->addWidget(m_fontchooser);

	pixkeySingle = Images::single();
	pixkeyDouble = Images::pair();
	addPage(m_page1, i18n("Encryption"), QLatin1String( "document-encrypt" ));
	addPage(m_page2, i18n("Decryption"), QLatin1String( "document-decrypt") );
	addPage(m_page3, i18n("Appearance"), QLatin1String( "preferences-desktop-theme" ));
	addPage(m_page4, i18n("GnuPG Settings"), QLatin1String( "kgpg" ));
	addPage(m_page6, i18n("Key Servers"), QLatin1String( "network-wired" ));
	addPage(m_page7, i18n("Misc"), QLatin1String( "preferences-other" ));

	// The following widgets are managed manually.
	connect(m_page1->encrypt_to_always, SIGNAL(toggled(bool)), this, SLOT(slotChangeEncryptTo()));
	connect(m_page4->changeHome, SIGNAL(clicked()), this, SLOT(slotChangeHome()));
	connect(m_page6->server_add, SIGNAL(clicked()), this, SLOT(slotAddKeyServer()));
	connect(m_page6->server_del, SIGNAL(clicked()), this, SLOT(slotDelKeyServer()));
	connect(m_page6->server_edit, SIGNAL(clicked()), this, SLOT(slotEditKeyServer()));
	connect(m_page6->server_default, SIGNAL(clicked()), this, SLOT(slotDefaultKeyServer()));
	connect(m_page6->ServerBox, SIGNAL(clicked(QModelIndex)), this, SLOT(slotEnableDeleteServer()));
	connect(m_page6->ServerBox, SIGNAL(executed(QListWidgetItem*)), this, SLOT(slotEditKeyServer(QListWidgetItem*)));
	connect(m_page7->kcfg_ShowSystray, SIGNAL(clicked()), SLOT(slotSystrayEnable()));

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
	delete m_page1;
	delete m_page2;
	delete m_page3;
	delete m_page4;
	delete m_page6;
	delete m_page7;
}

void kgpgOptions::slotChangeHome()
{
	QString gpgHome = KFileDialog::getExistingDirectory(m_page4->gpg_home_path->text(), this, i18n("New GnuPG Home Location"));
	if (gpgHome.isEmpty())
		return;

	if (!gpgHome.endsWith(QLatin1Char( '/' )))
		gpgHome.append(QLatin1Char( '/' ));

	QString confPath = QLatin1String( "options" );
	if (!QFile(gpgHome + confPath).exists()) {
		confPath = QLatin1String( "gpg.conf" );
		if (!QFile(gpgHome + confPath).exists()) {
			// Try to create config File by running gpg once
			if (KMessageBox::Yes == KMessageBox::questionYesNo(this,
					i18n("No configuration file was found in the selected location.\nDo you want to create it now?\n\nWithout a configuration file, neither KGpg nor GnuPG will work properly."),
					i18n("No Configuration File Found"),
					KGuiItem(i18n("Create")),
					KGuiItem(i18n("Ignore")))) {
				// start GnuPG so that it will create a config file
				QString gpgbin = m_page4->kcfg_GpgBinaryPath->text();
				if (!QFile::exists(gpgbin))
					gpgbin = QLatin1String( "gpg" );

				KProcess p;
				p << gpgbin << QLatin1String( "--homedir" ) << gpgHome << QLatin1String( "--no-tty" ) << QLatin1String( "--list-secret-keys" );
				p.execute();
				// end of creating config file

				confPath = QLatin1String( "gpg.conf" );
				QFile confFile(gpgHome + confPath);
				if (!confFile.open(QIODevice::WriteOnly)) {
					KMessageBox::sorry(this, i18n("Cannot create configuration file. Please check if destination media is mounted and if you have write access."));
					return;
				} else {
					QTextStream stream(&confFile);
					stream << "#  Config file created by KGpg\n\n";
					confFile.close();
				}
			} else {
				confPath.clear();
			}
		}
	}

	m_page4->gpg_conf_path->setText(confPath);
	m_page4->gpg_home_path->setText(gpgHome);
}

bool kgpgOptions::isValidKeyserver(const QString &server)
{
	if (server.isEmpty())
		return false;

	if (server.contains(QLatin1Char( ' ' ))) {
		KMessageBox::sorry(this, i18n("Key server URLs may not contain whitespace."));
		return false;
	}

	if (serverList.contains(server)) {
		KMessageBox::sorry(this, i18n("Key server already in the list."));
		return false;
	}

	return true;
}

void kgpgOptions::slotAddKeyServer()
{
	const QString newServer(KInputDialog::getText(i18n("Add New Key Server"), i18n("Server URL:")));

	if (!isValidKeyserver(newServer))
		return;

	serverList << newServer;
	QListWidgetItem *item = new QListWidgetItem(newServer, m_page6->ServerBox);
	m_page6->ServerBox->setCurrentItem(item);

	enableButtonApply(true);
}

void kgpgOptions::slotChangeEncryptTo()
{
	bool enable = (m_page1->encrypt_to_always->isChecked() != m_encrypttoalways);
	enableButtonApply(enable);
}

void kgpgOptions::slotDelKeyServer()
{
	QListWidgetItem *cur = m_page6->ServerBox->takeItem(m_page6->ServerBox->currentRow());
	bool defaultDeleted = cur->text().contains(QLatin1Char( ' ' ));

	// Are there any items left now we've took one out of the list?
	cur = m_page6->ServerBox->currentItem();
	if (cur == NULL) {
		// The list is empty so disable the delete button.
		m_page6->server_del->setEnabled(false);
		return;
	}

	cur->setSelected(true);
	if (defaultDeleted)
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

	QString oldServer(cur->text());
	bool isDefault = false;
	if (oldServer.contains(QLatin1Char( ' ' ))) {
		isDefault = true;
		oldServer = oldServer.section(QLatin1Char( ' ' ), 0, 0);
	}

	QString newServer(KInputDialog::getText(i18n("Edit Key Server"), i18n("Server URL:"), oldServer).simplified());
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
	if (!curr->text().contains(QLatin1Char( ' ' ))) {
		// The current item is not already the default one so a couple of things
		// must be changed now:
		// 1. The "(Default)" mark must be removed in the GUI list.
		// 2. keyServer must be updated to the new default server.
		// 3. The new default keyServer must have the "(Default)" mark in the GUI.
		if (m_page6->ServerBox->findItems(keyServer, Qt::MatchContains).size() > 0) {
			QListWidgetItem *prev = m_page6->ServerBox->findItems(keyServer, Qt::MatchContains).first();
			prev->setText(prev->text().remove(QLatin1Char( ' ' ) + i18nc("Mark default keyserver in GUI", "(Default)"))); // 1
		}

		keyServer = curr->text(); // 2
		curr->setText(i18nc("Mark default keyserver in GUI", "%1 (Default)", curr->text())); // 3

		enableButtonApply(true);
	}
}

void kgpgOptions::slotEnableDeleteServer()
{
	m_page6->server_del->setEnabled(true);
}

void kgpgOptions::updateWidgets()
{
	alwaysKeyID = KgpgInterface::getGpgSetting(QLatin1String( "encrypt-to" ), KGpgSettings::gpgConfigPath());

	m_page7->EmailTemplateEdit->setPlainText(KGpgSettings::emailTemplate());

	m_encrypttoalways = !alwaysKeyID.isEmpty();
	m_defaultencrypttoalways = false;

	m_page1->encrypt_to_always->setChecked(m_encrypttoalways);

	listKeys();
	fileEncryptionKey = KGpgSettings::fileEncryptionKey();
	// the contents are totally mess. There were key id, name and email stored
	// try to extract the key id, that's the only thing we really need
	if (!fileEncryptionKey.isEmpty()) {
		int idpos = m_page1->file_key->findText(fileEncryptionKey);
		if (idpos == -1) {
			idpos = fileEncryptionKey.indexOf(QRegExp( QLatin1String( "([0-9A-Fa-F]{8})+" )));
			if (idpos >= 0) {
				QString fileId = fileEncryptionKey.mid(idpos);
				idpos = fileId.indexOf(QRegExp( QLatin1String( "[^a-fA-F0-9]" )));
				if (idpos >= 0) {
					fileId = fileId.left(idpos);
					fileId.chop(fileId.length() % 8);
				}

				KGpgKeyNode *anode = m_combomodel->getModel()->findKeyNode(fileId);

				if (anode != NULL)
					idpos = m_combomodel->nodeIndex(anode).row();
			}
		}
		m_page1->file_key->setCurrentIndex(idpos);
	}

	if (!alwaysKeyID.isEmpty()) {
		KGpgKeyNode *anode = m_combomodel->getModel()->findKeyNode(alwaysKeyID);
		if (anode != NULL) {
			const QModelIndex midx(m_combomodel->nodeIndex(anode));
			m_page1->always_key->setCurrentIndex(midx.row());
		}
	}

	gpgConfigPath = KGpgSettings::gpgConfigPath();
	m_page4->gpg_conf_path->setText(KUrl::fromPath(gpgConfigPath).fileName());
	m_page4->gpg_home_path->setText(KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash));

	m_useagent = KgpgInterface::getGpgBoolSetting(QLatin1String( "use-agent" ), KGpgSettings::gpgConfigPath());
	m_defaultuseagent = false;

	m_page4->use_agent->setChecked(m_useagent);

	m_page6->ServerBox->clear();
	QStringList servers(serverList);

	if (!servers.isEmpty()) {
		const QString defaultServer(servers.takeFirst());
		servers.prepend(i18nc("Mark default keyserver in GUI", "%1 (Default)", defaultServer));
		m_page6->ServerBox->addItems(servers);
	}

	kDebug(2100) << "Finishing options" ;
}

void kgpgOptions::updateWidgetsDefault()
{
	m_page7->EmailTemplateEdit->setPlainText(m_emailTemplate);

	m_page1->encrypt_to_always->setChecked(m_defaultencrypttoalways);
	m_page4->use_agent->setChecked(m_defaultuseagent);

	m_page4->gpg_conf_path->setText(defaultConfigPath);
	m_page4->gpg_home_path->setText(defaultHomePath);

	m_page6->ServerBox->clear();
	m_page6->ServerBox->addItem(i18nc("Mark default keyserver in GUI", "%1 (Default)", defaultKeyServer));
	m_page6->ServerBox->addItems(defaultServerList);

	kDebug(2100) << "Finishing default options" ;
}

void kgpgOptions::updateSettings()
{
	// Update config path first!
	const QString newConfigFile = m_page4->gpg_home_path->text() + m_page4->gpg_conf_path->text();
	if (newConfigFile != gpgConfigPath) {
		KGpgSettings::setGpgConfigPath(newConfigFile);
		emit homeChanged();

		gpgConfigPath = newConfigFile;
	}

	// save selected keys for file encryption & always encrypt with
	if (m_page1->kcfg_EncryptFilesTo->isChecked())
		fileEncryptionKey = m_page1->file_key->itemData(m_page1->file_key->currentIndex(), Qt::ToolTipRole).toString();
	else
		fileEncryptionKey.clear();

	if (fileEncryptionKey != KGpgSettings::fileEncryptionKey())
		KGpgSettings::setFileEncryptionKey(fileEncryptionKey);

	m_encrypttoalways = m_page1->encrypt_to_always->isChecked();

	if (m_encrypttoalways)
		alwaysKeyID = m_page1->always_key->itemData(m_page1->always_key->currentIndex(), Qt::ToolTipRole).toString();
	else
		alwaysKeyID.clear();

	KgpgInterface::setGpgSetting(QLatin1String( "encrypt-to" ), alwaysKeyID, KGpgSettings::gpgConfigPath());

	emit changeFont(m_fontchooser->font());

	// install service menus
	if (m_page7->kcfg_SignMenu->currentIndex() == KGpgSettings::EnumSignMenu::AllFiles)
		slotInstallSign(QLatin1String( "application/octet-stream" ));
	else
		slotRemoveMenu(QLatin1String( "signfile.desktop" ));

	if (m_page7->kcfg_DecryptMenu->currentIndex() == KGpgSettings::EnumDecryptMenu::AllFiles)
		slotInstallDecrypt(QLatin1String( "application/octet-stream" ));
	else
	if (m_page7->kcfg_DecryptMenu->currentIndex() == KGpgSettings::EnumDecryptMenu::EncryptedFiles)
		slotInstallDecrypt(QLatin1String( "application/pgp-encrypted,application/pgp-signature,application/pgp-keys" ));
	else
		slotRemoveMenu(QLatin1String( "decryptfile.desktop" ));

	m_useagent = m_page4->use_agent->isChecked();

	if (m_useagent) {
		KgpgInterface::setGpgBoolSetting(QLatin1String( "use-agent" ), true, KGpgSettings::gpgConfigPath());
		KgpgInterface::setGpgBoolSetting(QLatin1String( "no-use-agent" ), false, KGpgSettings::gpgConfigPath());
	} else {
		KgpgInterface::setGpgBoolSetting(QLatin1String( "use-agent" ), false, KGpgSettings::gpgConfigPath());
	}

	// Store the default server in ~/.gnupg
	KgpgInterface::setGpgSetting(QLatin1String( "keyserver" ), keyServer, KGpgSettings::gpgConfigPath());

	// Store additional servers in kgpgrc.
	serverList.clear();
	for (int i = 0; i < m_page6->ServerBox->count(); i++) {
		QString server(m_page6->ServerBox->item(i)->text());

		// Only store the additional servers in the config file.
		if (!server.contains(QLatin1Char( ' ' ))) {
			serverList.append(server);
		} else {
			server.remove(QRegExp( QLatin1String( " .*")) );	// Remove the " (Default)" ) section.
			serverList.prepend(server);		// Make it the first item in the list.
		}
	}
	KGpgSettings::setKeyServers(serverList);

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

	m_trayaction = m_page7->kcfg_LeftClick->currentIndex();
	KGpgSettings::setLeftClick(m_trayaction);

	m_mailUats = m_page7->kcfg_MailUats->currentIndex();
	KGpgSettings::setMailUats(m_mailUats);

	m_emailTemplate = m_page7->EmailTemplateEdit->toPlainText();
	KGpgSettings::setEmailTemplate(m_emailTemplate);

	KGpgSettings::self()->writeConfig();
	m_config->sync();

	emit settingsUpdated();
}

void kgpgOptions::listKeys()
{
	if (m_model->rowCount(QModelIndex()) == 0) {
		ids += QLatin1String("0");
		m_page1->file_key->addItem(i18nc("no key available", "none"));
		m_page1->file_key->setModel(NULL);
		m_page1->always_key->addItem(i18nc("no key available", "none"));
		m_page1->always_key->setModel(NULL);
	} else {
		m_page1->file_key->setModel(m_combomodel);
		m_page1->always_key->setModel(m_combomodel);
	}
}

void kgpgOptions::slotInstallDecrypt(const QString &mimetype)
{
	const QString path(KStandardDirs::locateLocal("data", QLatin1String( "konqueror/servicemenus/decryptfile.desktop" )));
	KDesktopFile configl2(path);
	if (!configl2.isImmutable()) {
		KConfigGroup gr(configl2.group("Desktop Entry"));

		gr.writeXdgListEntry("MimeType", QStringList(mimetype));
		gr.writeEntry("X-KDE-ServiceTypes", "KonqPopupMenu/Plugin");
		gr.writeEntry("Actions", "decrypt");

		gr = configl2.group("Desktop Action decrypt");
		gr.writeEntry("Name", i18n("Decrypt File"));
		//gr.writeEntry("Icon", "decrypt_file");
		gr.writeEntry("Exec", "kgpg %U");
	}
}

void kgpgOptions::slotInstallSign(const QString &mimetype)
{
	QString path(KStandardDirs::locateLocal("services", QLatin1String( "ServiceMenus/signfile.desktop" )));
	KDesktopFile configl2(path);
	if (!configl2.isImmutable()) {
		KConfigGroup gr = configl2.group("Desktop Entry");
		gr.writeXdgListEntry("MimeType", QStringList(mimetype));
		gr.writeEntry("X-KDE-ServiceTypes", "KonqPopupMenu/Plugin");
		gr.writeEntry("Actions", "sign");

		gr = configl2.group("Desktop Action sign");
		gr.writeEntry("Name", i18n("Sign File"));
		//gr.writeEntry("Icon", "sign_file");
		gr.writeEntry("Exec","kgpg -S %F");
	}
}

void kgpgOptions::slotRemoveMenu(const QString &menu)
{
	QString path = KStandardDirs::locateLocal("services", QLatin1String( "ServiceMenus/" ) + menu);
	QFile qfile(path);
	if (qfile.exists())
		qfile.remove();
}

bool kgpgOptions::hasChanged()
{
	if (m_page1->kcfg_EncryptFilesTo->isChecked() && (m_page1->file_key->currentText() != fileEncryptionKey))
		return true;

	if (m_page1->encrypt_to_always->isChecked() != m_encrypttoalways)
		return true;

	if (m_page1->encrypt_to_always->isChecked() &&
			(m_page1->always_key->itemData(m_page1->always_key->currentIndex(), Qt::ToolTipRole).toString()) != alwaysKeyID)
		return true;

	if (m_page4->gpg_conf_path->text() != KUrl::fromPath(gpgConfigPath).fileName())
		return true;

	if (m_page4->gpg_home_path->text() != KUrl::fromPath(gpgConfigPath).directory(KUrl::AppendTrailingSlash))
		return true;

	if (m_page4->use_agent->isChecked() != m_useagent)
		return true;

	// Did the number of servers change?
	if (m_page6->ServerBox->count() != serverList.size())
		return true;

	// Did the actual value of the servers change than?
	for (int i = 0; i < m_page6->ServerBox->count(); i++) {
		QString server(m_page6->ServerBox->item(i)->text());

		if (server.contains(QLatin1Char( ' ' ))) {
			// This is the current server marked as default in the GUI.
			server.remove(QLatin1Char( ' ' ) + i18nc( "Remove default marker from GUI if it is there", "(Default)"));
			if (keyServer != server)
				return true;
		} else if (!serverList.contains(server)) {
			return true;
		}
	}

	if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
		return true;

	if (m_page7->kcfg_LeftClick->currentIndex() != m_trayaction)
		return true;

	if (m_page7->kcfg_MailUats->currentIndex() != m_mailUats)
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

	if (m_page4->use_agent->isChecked() != m_defaultuseagent)
		return false;

	for (int i = 0; i < m_page6->ServerBox->count(); i++) {
		QString server(m_page6->ServerBox->item(i)->text());

		if (server.contains(QLatin1Char( ' ' ))) {
			// This is the current server marked as default in the GUI.
			server.remove(QLatin1Char( ' ' ) + i18nc( "Remove default marker from GUI if it is there", "(Default)"));
			if (defaultKeyServer != server)
				return false;
		} else if (!defaultServerList.contains(server)) {
			return false;
		}
	}

	if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
		return false;

	if (m_page7->kcfg_LeftClick->currentIndex() != KGpgSettings::EnumLeftClick::KeyManager)
		return false;

	if (m_page7->kcfg_MailUats->currentIndex() != KGpgSettings::EnumMailUats::All)
		return false;

	return true;
}

void kgpgOptions::slotSystrayEnable()
{
	m_page7->kcfg_LeftClick->setEnabled(m_page7->kcfg_ShowSystray->isChecked());
}

#include "kgpgoptions.moc"
