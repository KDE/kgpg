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

// code for the option dialog box
#include <stdlib.h>

#include <QTextStream>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFile>
#include <QFont>

#include <kstandarddirs.h>
#include <kinputdialog.h>
#include <kdesktopfile.h>
#include <kcolorbutton.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kfontdialog.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kprocio.h>

#include "kgpgsettings.h"
#include "kgpgoptions.h"

#include "conf_decryption.h"
#include "conf_encryption.h"
#include "conf_gpg.h"
#include "conf_servers.h"
#include "conf_ui2.h"
#include "conf_misc.h"

//   main window
kgpgOptions::kgpgOptions(QWidget *parent, const char *name)
           : KConfigDialog(parent, name, KGpgSettings::self())
{
    m_config = new KSimpleConfig("kgpgrc");

    defaultServerList = "hkp://wwwkeys.eu.pgp.net ";
    defaultServerList += i18n("(Default)");
    defaultServerList += ",hkp://search.keyserver.net,hkp://wwwkeys.pgp.net,hkp://pgp.dtype.org,hkp://wwwkeys.us.pgp.net";

    m_config->setGroup("Servers");
    serverList = m_config->readEntry("Server_List", defaultServerList).split(",");
    keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());

    if (!keyServer.isEmpty())
        serverList.prepend(keyServer + " " + i18n("(Default)"));

    defaultHomePath = QDir::homePath() + "/.gnupg/";
    if (QFile(defaultHomePath + "options").exists())
        defaultConfigPath = "options";
    else
    if (QFile(defaultHomePath + "gpg.conf").exists())
        defaultConfigPath = "gpg.conf";
    else
        defaultConfigPath = QString::null;

    kDebug(2100) << "Adding pages" << endl;
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

    m_page7->shredInfo->setText(i18n( "<qt><p>You must be aware that <b>shredding is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>"));
    m_page7->groupShred->adjustSize();

    pixkeySingle = KGlobal::iconLoader()->loadIcon("kgpg_key1", K3Icon::Small, 20);
    pixkeyDouble = KGlobal::iconLoader()->loadIcon("kgpg_key2", K3Icon::Small, 20);
    addPage(m_page1, i18n("Encryption"), "encrypted");
    addPage(m_page2, i18n("Decryption"), "decrypted");
    addPage(m_page3, i18n("Appearance"), "looknfeel");
    addPage(m_page4, i18n("GnuPG Settings"), "kgpg");
    addPage(m_page6, i18n("Key Servers"), "network");
    addPage(m_page7, i18n("Misc"), "misc");

    // The following widgets are managed manually.
    connect(m_page1->encrypt_to_always, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
    connect(m_page1->file_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
    connect(m_page1->always_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
    connect(m_page4->gpg_conf_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
    connect(m_page4->gpg_home_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
    connect(m_page4->use_agent, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
    connect(m_page4->changeHome, SIGNAL(clicked()), this, SLOT(slotChangeHome()));
    connect(m_page6->server_add, SIGNAL(clicked()), this, SLOT(slotAddKeyServer()));
    connect(m_page6->server_del, SIGNAL(clicked()), this, SLOT(slotDelKeyServer()));
    //connect(m_page6->server_edit, SIGNAL(clicked()), this, SLOT(slotEditKeyServer()));
    connect(m_page6->server_default, SIGNAL(clicked()), this, SLOT(slotDefaultKeyServer()));
    connect(m_page6->ServerBox, SIGNAL(currentChanged(Q3ListBoxItem *)), this, SLOT(updateButtons()));
    connect(m_page7->pushShredder, SIGNAL(clicked ()), this, SIGNAL(installShredder()));

    keyGood = KGpgSettings::colorGood();
    keyUnknown = KGpgSettings::colorUnknown();
    keyRev = KGpgSettings::colorRev();
    keyBad = KGpgSettings::colorBad();
}

kgpgOptions::~kgpgOptions()
{
}

void kgpgOptions::slotChangeHome()
{
    QString gpgHome = KFileDialog::getExistingDirectory(m_page4->gpg_home_path->text(), this, i18n("New GnuPG Home Location"));
    if (gpgHome.isEmpty())
        return;

    if (!gpgHome.endsWith("/"))
        gpgHome.append("/");

    QString confPath = "options";
    if (!QFile(gpgHome + confPath).exists())
    {
        confPath = "gpg.conf";
        if (!QFile(gpgHome + confPath).exists())
        {
            if (KMessageBox::questionYesNo(this, i18n("No configuration file was found in the selected location.\nDo you want to create it now ?\n\nWithout configuration file, neither KGpg nor Gnupg will work properly."), i18n("No Configuration File Found"), KGuiItem(i18n("Create")), KGuiItem(i18n("Ignore"))) == KMessageBox::Yes) // Try to create config File by running gpg once
            {
                // start gnupg so that it will create a config file
                KProcIO *p = new KProcIO();
                *p << "gpg" << "--homedir" << gpgHome << "--no-tty" << "--list-secret-keys";
                p->start(KProcess::Block);
                delete p;
                // end of creating config file

                confPath = "gpg.conf";
                QFile confFile(gpgHome + confPath);
                if (!confFile.open(QIODevice::WriteOnly))
                {
                    KMessageBox::sorry(this, i18n("Cannot create configuration file. Please check if destination media is mounted and if you have write access"));
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
                confPath = QString::null;
        }
    }

    m_page4->gpg_conf_path->setText(confPath);
    m_page4->gpg_home_path->setText(gpgHome);
}

void kgpgOptions::slotAddKeyServer()
{
    QString newServer = KInputDialog::getText(i18n("Add New Key Server"), i18n("Server URL:"));
    if (!newServer.isEmpty())
        m_page6->ServerBox->insertItem(newServer.simplified());

    m_page6->ServerBox->setSelected(m_page6->ServerBox->findItem(newServer.simplified()), true);
}

void kgpgOptions::slotDelKeyServer()
{
    bool defaultDeleted = false;
    if (m_page6->ServerBox->currentText().contains( ' ' ) )
        defaultDeleted = true;

    m_page6->ServerBox->removeItem(m_page6->ServerBox->currentItem());
    m_page6->ServerBox->setSelected(0, true);

    if (defaultDeleted)
        m_page6->ServerBox->changeItem(m_page6->ServerBox->currentText().section(" ", 0, 0) + " " + i18n("(Default)"), 0);
}

void kgpgOptions::slotEditKeyServer()
{
	QString oldServer = m_page6->ServerBox->currentText();
	bool isDefault = false;
	if (oldServer.contains(' ')) {
		isDefault = true;
		oldServer = oldServer.section(" ", 0, 0);
	}

	QString newServer = KInputDialog::getText(i18n("Edit Key Server"), i18n("Server URL:"), oldServer).simplified();
	if (newServer.isEmpty())
		return;
	if (isDefault)
		newServer = newServer + " " + i18n("(Default)");
	m_page6->ServerBox->changeItem(newServer, m_page6->ServerBox->currentItem());
}

void kgpgOptions::slotDefaultKeyServer()
{
    uint curr = m_page6->ServerBox->currentItem();
    m_page6->ServerBox->changeItem(m_page6->ServerBox->currentText().section(" ", 0, 0) + " " + i18n("(Default)"), curr);

    for (uint i = 0; i < m_page6->ServerBox->count(); i++)
        if (i != curr)
            m_page6->ServerBox->changeItem(m_page6->ServerBox->text(i).section(" ", 0, 0), i);

    m_page6->ServerBox->setSelected(curr, true);
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

    m_useagent = KgpgInterface::getGpgBoolSetting("use-agent", KGpgSettings::gpgConfigPath());
    m_defaultuseagent = false;

    m_page4->use_agent->setChecked(m_useagent);

    keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());
    defaultKeyServer = "hkp://wwwkeys.pgp.net";

    if (keyServer.isEmpty())
        keyServer = defaultKeyServer;

    m_page6->ServerBox->clear();
    m_page6->ServerBox->insertStringList(serverList);

    kDebug(2100) << "Finishing options" << endl;
}

void kgpgOptions::updateWidgetsDefault()
{
    m_page1->encrypt_to_always->setChecked(m_defaultencrypttoalways);
    m_page4->use_agent->setChecked(m_defaultuseagent);

    m_page4->gpg_conf_path->setText(defaultConfigPath);
    m_page4->gpg_home_path->setText(defaultHomePath);

    m_page6->ServerBox->clear();
    m_page6->ServerBox->insertStringList(defaultServerList.split(","));

    kDebug(2100) << "Finishing default options" << endl;
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

    // save selected keys for file encryption & always encrypt with
    if (m_page1->kcfg_EncryptFilesTo->isChecked())
        fileEncryptionKey = m_page1->file_key->currentText();
    else
        fileEncryptionKey = QString::null;

    if (fileEncryptionKey != KGpgSettings::fileEncryptionKey())
        KGpgSettings::setFileEncryptionKey(fileEncryptionKey);

    m_encrypttoalways = m_page1->encrypt_to_always->isChecked();

    if (m_encrypttoalways)
        alwaysKeyID = m_page1->always_key->currentText().section(':', 0, 0);
    else
        alwaysKeyID = QString::null;

    KgpgInterface::setGpgSetting("encrypt-to", alwaysKeyID, KGpgSettings::gpgConfigPath());

    emit changeFont(m_fontchooser->font());

    // install service menus
    if (m_page7->kcfg_SignMenu->currentIndex() == KGpgSettings::EnumSignMenu::AllFiles)
        slotInstallSign("all/allfiles");
    else
        slotRemoveMenu("signfile.desktop");

    if (m_page7->kcfg_DecryptMenu->currentIndex() == KGpgSettings::EnumDecryptMenu::AllFiles)
        slotInstallDecrypt("all/allfiles");
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

    QString currList;
    serverList = QStringList();
    for (uint i = 0; i < m_page6->ServerBox->count(); i++)
    {
        QString currItem = m_page6->ServerBox->text(i);
        if (currItem.contains(' ')) // it is the default keyserver
            keyServer = currItem.section(" ", 0, 0);
        else
            serverList.append(currItem);
    }

    currList = serverList.join(",");
    m_config->setGroup("Servers");
    m_config->writeEntry("Server_List", currList);

    KgpgInterface::setGpgSetting("keyserver", keyServer, KGpgSettings::gpgConfigPath());
    serverList.prepend(keyServer + " " + i18n("(Default)"));

    if (keyGood != m_page3->kcfg_ColorGood->color())
        emit refreshTrust(GoodColor, m_page3->kcfg_ColorGood->color());

    if (keyBad != m_page3->kcfg_ColorBad->color())
        emit refreshTrust(BadColor, m_page3->kcfg_ColorBad->color());

    if (keyUnknown != m_page3->kcfg_ColorUnknown->color())
        emit refreshTrust(UnknownColor, m_page3->kcfg_ColorUnknown->color());

    if (keyRev != m_page3->kcfg_ColorRev->color())
        emit refreshTrust(RevColor, m_page3->kcfg_ColorRev->color());

    KGpgSettings::writeConfig();
    m_config->sync();
    emit settingsUpdated();
}

void kgpgOptions::listKeys()
{
    // update display of keys in ComboBox
    QString line;
    QString name;
    QString trustedvals = "idre-";
    QString issec;
    int counter = 0;

    KProcIO *p = new KProcIO();
    *p << "gpg" << "--no-secmem-warning" << "--no-tty" << "--with-colon" << "--list-secret-keys";
    p->start(KProcess::Block, true);

    while (p->readln(line) != -1)
        if (line.startsWith("sec"))
            issec += line.section(':', 4, 4);

    delete p;

    p = new KProcIO();
    *p << "gpg" << "--no-tty" << "--with-colon" << "--list-keys";
    p->start(KProcess::Block, true);

    while (p->readln(line) != -1)
    {
        if (line.startsWith("pub"))
        {
            name = KgpgInterface::checkForUtf8(line.section(':', 9, 9));
            if ((!name.isEmpty()) && !trustedvals.contains(line.section(':', 1, 1) ) )
            {
                counter++;
                //name=name.section('<',-1,-1);
                //  name=name.section('>',0,0);
                names += name;
                ids += line.section(':', 4, 4);
                if (line.section(':', 4, 4).right(8) == alwaysKeyID)
                    alwaysKeyName = line.section(':', 4, 4).right(8) + ":" + name;

                if (issec.contains(line.section(':', 4, 4).right(8), Qt::CaseInsensitive ) )
                {
                    m_page1->file_key->addItem(pixkeyDouble, line.section(':', 4, 4).right(8) + ":" + name);
                    m_page1->always_key->addItem(pixkeyDouble, line.section(':', 4, 4).right(8) + ":" + name);
                }
                else
                {
                    m_page1->file_key->addItem(pixkeySingle, line.section(':', 4, 4).right(8) + ":" + name);
                    m_page1->always_key->addItem(pixkeySingle, line.section(':', 4, 4).right(8) + ":" + name);
                }
            }
        }
    }

    delete p;

    if (counter == 0)
    {
        ids += "0";
        m_page1->file_key->addItem(i18n("none"));
        m_page1->always_key->addItem(i18n("none"));
    }
}

void kgpgOptions::slotInstallDecrypt(QString mimetype)
{
    QString path = KStandardDirs::locateLocal("data", "konqueror/servicemenus/decryptfile.desktop");
    KDesktopFile configl2(path, false);
    if (configl2.isImmutable() == false)
    {
        configl2.setGroup("Desktop Entry");
        configl2.writeEntry("ServiceTypes", mimetype);
        configl2.writeEntry("Actions", "decrypt");
        configl2.setGroup("Desktop Action decrypt");
        configl2.writeEntry("Name", i18n("Decrypt File"));
        //configl2.writeEntry("Icon", "decrypt_file");
        configl2.writeEntry("Exec", "kgpg %U");
        //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
    }
}

void kgpgOptions::slotInstallSign(QString mimetype)
{
    QString path = KStandardDirs::locateLocal("data", "konqueror/servicemenus/signfile.desktop");
    KDesktopFile configl2(path, false);
    if (configl2.isImmutable() ==false)
    {
        configl2.setGroup("Desktop Entry");
        configl2.writeEntry("ServiceTypes", mimetype);
        configl2.writeEntry("Actions", "sign");
        configl2.setGroup("Desktop Action sign");
        configl2.writeEntry("Name", i18n("Sign File"));
        //configl2.writeEntry("Icon", "sign_file");
        configl2.writeEntry("Exec","kgpg -S %F");
        //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
    }
}

void kgpgOptions::slotRemoveMenu(QString menu)
{
    QString path = KStandardDirs::locateLocal("data", "konqueror/servicemenus/" + menu);
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

    if (m_page4->use_agent->isChecked() != m_useagent)
        return true;

    QStringList currList;
    for (uint i = 0; i < m_page6->ServerBox->count(); i++)
        currList.append(m_page6->ServerBox->text(i));

    if (currList != serverList)
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

    QString currList;
    for (uint i = 0; i < m_page6->ServerBox->count(); i++)
        currList += m_page6->ServerBox->text(i) + ",";
    currList.truncate(currList.length() - 1);

    if (currList != defaultServerList)
        return false;

    return true;
}

#include "kgpgoptions.moc"
