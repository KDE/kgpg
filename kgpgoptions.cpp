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

#include "kgpgoptions.h"

#include "kgpgsettings.h"
#include "kgpginterface.h"
#include "conf_encryption.h"
#include "core/images.h"
#include "model/gpgservermodel.h"
#include "model/keylistproxymodel.h"
#include "model/kgpgitemmodel.h"

#include <KConfig>
#include <KDesktopFile>
#include <KFileDialog>
#include <KFontChooser>
#include <KInputDialog>
#include <KLocale>
#include <KMessageBox>
#include <KProcess>
#include <KStandardDirs>
#include <QCheckBox>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <kdefakes.h>

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
	   m_serverModel(new GpgServerModel(m_page6)),
	   m_fontchooser(new KFontChooser(m_page3->tabWidget3->widget(1), KFontChooser::NoDisplayFlags, QStringList())),
	   m_model(model),
	   m_combomodel(new KeyListProxyModel(this, KeyListProxyModel::SingleColumnIdFirst))
{
	m_page7->EmailTemplateEdit->setPlainText(KGpgSettings::emailTemplate());

	m_combomodel->setKeyModel(m_model);
	m_combomodel->setTrustFilter(KgpgCore::TRUST_MARGINAL);
	m_combomodel->setEncryptionKeyFilter(true);
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
	serverList.replaceInStrings(QRegExp( QLatin1String( " .*") ), QString() );

	m_serverModel->setStringList(serverList);
	// if the server from GnuPG config is set and is not in the list of servers put it there
	if (!keyServer.isEmpty() && !serverList.contains(keyServer))
		serverList.prepend(keyServer);
	m_page6->ServerBox->setModel(m_serverModel);

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

	m_page3->kcfg_EmailSorting->addItem(i18n("Left to right, account first")); ///< KGpgSettings::EnumEmailSorting::Alphabetical
	m_page3->kcfg_EmailSorting->addItem(i18n("Right to left, TLD first")); ///< KGpgSettings::EnumEmailSorting::TLDfirst
	m_page3->kcfg_EmailSorting->addItem(i18n("Right to left, domain first")); ///< KGpgSettings::EnumEmailSorting::DomainFirst
	m_page3->kcfg_EmailSorting->addItem(i18n("Right to left, FQDN first")); ///< KGpgSettings::EnumEmailSorting::FQDNFirst
	m_emailSortingIndex = KGpgSettings::emailSorting();

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
	connect(m_page6->ServerBox->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(slotChangeKeyServerButtonEnable()));
	connect(m_page6->ServerBox, SIGNAL(doubleClicked(QModelIndex)), SLOT(slotEditKeyServer(QModelIndex)));
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

	m_serverModel->setStringList(m_serverModel->stringList() << newServer);

	enableButtonApply(true);
}

void kgpgOptions::slotChangeEncryptTo()
{
	bool enable = (m_page1->encrypt_to_always->isChecked() != m_encrypttoalways);
	enableButtonApply(enable);
}

void kgpgOptions::slotDelKeyServer()
{
	QModelIndex cur = m_page6->ServerBox->selectionModel()->currentIndex();
	m_serverModel->removeRows(cur.row(), 1);

	enableButtonApply(true);
}

void kgpgOptions::slotEditKeyServer()
{
	slotEditKeyServer(m_page6->ServerBox->selectionModel()->currentIndex());
}

void kgpgOptions::slotEditKeyServer(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	m_page6->ServerBox->edit(index);

	enableButtonApply(true);
}

void kgpgOptions::slotDefaultKeyServer()
{
	QModelIndex cur = m_page6->ServerBox->selectionModel()->currentIndex();

	m_serverModel->setDefault(cur.row());

	enableButtonApply(true);
}

void kgpgOptions::slotChangeKeyServerButtonEnable()
{
	QModelIndex cur = m_page6->ServerBox->selectionModel()->currentIndex();

	m_page6->server_del->setEnabled(cur.isValid());
	m_page6->server_edit->setEnabled(cur.isValid());
	m_page6->server_default->setEnabled(cur.isValid() &&
			(cur.row() != m_serverModel->defaultRow()));
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

	m_emailSortingIndex = KGpgSettings::emailSorting();
	m_page3->kcfg_EmailSorting->setCurrentIndex(m_emailSortingIndex);

	m_serverModel->setStringList(serverList);
	if (!serverList.isEmpty())
		m_serverModel->setDefault(keyServer);

	kDebug(2100) << "Finishing options";
}

void kgpgOptions::updateWidgetsDefault()
{
	m_page7->EmailTemplateEdit->setPlainText(m_emailTemplate);

	m_page1->encrypt_to_always->setChecked(m_defaultencrypttoalways);
	m_page4->use_agent->setChecked(m_defaultuseagent);

	m_page4->gpg_conf_path->setText(defaultConfigPath);
	m_page4->gpg_home_path->setText(defaultHomePath);

	m_serverModel->setStringList(defaultServerList);
	m_serverModel->setDefault(0);

	m_page3->kcfg_EmailSorting->setCurrentIndex(KGpgSettings::EnumEmailSorting::Alphabetical);

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
	KgpgInterface::setGpgSetting(QLatin1String("keyserver"), m_serverModel->defaultServer(), KGpgSettings::gpgConfigPath());

	// Store additional servers in kgpgrc.
	serverList = m_serverModel->stringList();
	int defaultRow = m_serverModel->defaultRow();
	if (!serverList.isEmpty())
		serverList.move(defaultRow, 0);

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

	m_emailSortingIndex = m_page3->kcfg_EmailSorting->currentIndex();
	KGpgSettings::setEmailSorting(m_emailSortingIndex);

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

	// Did the default server change
	if (m_serverModel->defaultServer() != keyServer)
		return true;

	// Did the servers change?
	if (m_serverModel->stringList() != serverList)
		return true;

	if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
		return true;

	if (m_page7->kcfg_LeftClick->currentIndex() != m_trayaction)
		return true;

	if (m_page7->kcfg_MailUats->currentIndex() != m_mailUats)
		return true;

	if (m_page3->kcfg_EmailSorting->currentIndex() != m_emailSortingIndex)
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

	if (m_serverModel->defaultServer() != defaultKeyServer)
		return false;

	if (m_serverModel->stringList() != defaultServerList)
		return false;

	if (m_page7->kcfg_ShowSystray->isChecked() != m_showsystray)
		return false;

	if (m_page7->kcfg_LeftClick->currentIndex() != KGpgSettings::EnumLeftClick::KeyManager)
		return false;

	if (m_page7->kcfg_MailUats->currentIndex() != KGpgSettings::EnumMailUats::All)
		return false;

	if (m_page3->kcfg_EmailSorting->currentIndex() != KGpgSettings::EnumEmailSorting::Alphabetical)
		return false;

	return true;
}

void kgpgOptions::slotSystrayEnable()
{
	m_page7->kcfg_LeftClick->setEnabled(m_page7->kcfg_ShowSystray->isChecked());
}

#include "kgpgoptions.moc"
