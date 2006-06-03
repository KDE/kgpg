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

///////////////////////////////////////////////             code for the option dialog box

#include <config.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qfile.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kprocio.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qfont.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kfontdialog.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <kcolorbutton.h>
#include <stdlib.h>
#include <kdebug.h>
#include <kactivelabel.h>

#include "kgpgoptions.h"
#include "kgpgsettings.h"
#include "listkeys.h"

#include "conf_decryption.h"
#include "conf_encryption.h"
#include "conf_gpg.h"
#include "conf_servers.h"
#include "conf_ui2.h"
#include "conf_misc.h"

class QTabWidget;

///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name)
 : KConfigDialog( parent, name, KGpgSettings::self())
{
	defaultServerList="hkp://wwwkeys.eu.pgp.net ";
	defaultServerList+=i18n("(Default)");
	defaultServerList+=",hkp://search.keyserver.net,hkp://wwwkeys.pgp.net,hkp://pgp.dtype.org,hkp://wwwkeys.us.pgp.net";
	config = new KConfig ("kgpgrc");
	config->setGroup("Servers");
	serverList=QStringList::split (",",config->readEntry("Server_List",defaultServerList));
	keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());

	if (!keyServer.isEmpty()) serverList.prepend(keyServer+" "+i18n("(Default)"));

	defaultHomePath=QDir::homeDirPath()+"/.gnupg/";
	if (QFile(defaultHomePath+"options").exists()) defaultConfigPath="options";
	else
	{
                if (QFile(defaultHomePath+"gpg.conf").exists()) defaultConfigPath="gpg.conf";
		else defaultConfigPath=QString::null;
		}

kdDebug(2100)<<"Adding pages"<<endl;
        page1=new Encryption();
        page2=new Decryption();
        page3=new UIConf();
        page4=new GPGConf();
	page6=new ServerConf();
	page7=new MiscConf();
	QBoxLayout *fontLayout=new QBoxLayout(page3->tabWidget3->page(1),QBoxLayout::TopToBottom,10);
	kfc=new KFontChooser(page3->tabWidget3->page(1),"kcfg_Font",false,QStringList(),false);
	fontLayout->addWidget(kfc);

	page7->shredInfo->setText(i18n( "<qt><p>You must be aware that <b>shredding is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>"));
	page7->groupShred->adjustSize();
	pixkeySingle=KGlobal::iconLoader()->loadIcon("kgpg_key1",KIcon::Small,20);
	pixkeyDouble=KGlobal::iconLoader()->loadIcon("kgpg_key2",KIcon::Small,20);
        addPage(page1, i18n("Encryption"), "encrypted");
        addPage(page2, i18n("Decryption"), "decrypted");
        addPage(page3, i18n("Appearance"), "looknfeel");
        addPage(page4, i18n("GnuPG Settings"), "kgpg");
	addPage(page6, i18n("Key Servers"), "network");
	addPage(page7, i18n("Misc"), "misc");

	page1->clear_akey->setIconSet(QIconSet(QPixmap(SmallIcon("clear_left"))));
	page1->clear_fkey->setIconSet(QIconSet(QPixmap(SmallIcon("clear_left"))));

        // The following widgets are managed manually.
        connect(page1->change_fkey, SIGNAL(clicked()), this, SLOT(insertFileKey()));
	connect(page1->clear_fkey, SIGNAL(clicked()), page1->kcfg_FileKey, SLOT(clear()));
	connect(page1->change_akey, SIGNAL(clicked()), this, SLOT(insertAlwaysKey()));
	connect(page1->clear_akey, SIGNAL(clicked()), page1->alwaysKey, SLOT(clear()));
	connect(page1->alwaysKey, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
	connect(page4->gpg_conf_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
	connect(page4->gpg_home_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
        connect(page4->use_agent, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(page4->changeHome, SIGNAL(clicked()), this, SLOT(slotChangeHome()));
	connect(page4->kcfg_PubKeyring, SIGNAL(toggled (bool)), page4->kcfg_PubKeyringUrl, SLOT(setEnabled(bool)));
	connect(page4->kcfg_PubKeyring, SIGNAL(toggled (bool)), this, SLOT(checkAdditionalState(bool)));
	connect(page4->kcfg_PrivKeyring, SIGNAL(toggled (bool)), page4->kcfg_PrivKeyringUrl, SLOT(setEnabled(bool)));
	connect(page4->kcfg_PrivKeyring, SIGNAL(toggled (bool)), this, SLOT(checkAdditionalState(bool)));
	connect(page6->server_add, SIGNAL(clicked()), this, SLOT(slotAddKeyServer()));
	connect(page6->server_del, SIGNAL(clicked()), this, SLOT(slotDelKeyServer()));
	connect(page6->server_default, SIGNAL(clicked()), this, SLOT(slotDefaultKeyServer()));
	connect(page6->ServerBox, SIGNAL(currentChanged ( QListBoxItem *)), this, SLOT(updateButtons()));
	connect(page7->pushShredder, SIGNAL(clicked ()), this, SIGNAL(installShredder()));

	//connect(this, SIGNAL(settingsChanged()), SLOT(updateSettings()));

	keyGood=KGpgSettings::colorGood();
	keyUnknown=KGpgSettings::colorUnknown();
	keyRev=KGpgSettings::colorRev();
	keyBad=KGpgSettings::colorBad();
}


kgpgOptions::~kgpgOptions()
{
delete config;
}


void kgpgOptions::checkAdditionalState(bool)
{
// enable / disable the "use only this keyring" option depending on the other checkboxes state

if (page4->kcfg_PubKeyring->isOn() && page4->kcfg_PrivKeyring->isOn())
page4->kcfg_OnlyAdditional->setEnabled(true);
else
page4->kcfg_OnlyAdditional->setEnabled(false);
}

void kgpgOptions::insertFileKey()
{
		QString signKeyID;
		///// open key selection dialog
                KgpgSelKey *opts=new KgpgSelKey(this,0,true,page1->kcfg_FileKey->text());

                if (opts->exec()==QDialog::Accepted) {
                        page1->kcfg_FileKey->setText(opts->getkeyID());
                } else {
                        delete opts;
                        return;
                }
                delete opts;
}

void kgpgOptions::insertAlwaysKey()
{
		QString signKeyID;
		///// open key selection dialog
                KgpgSelKey *opts=new KgpgSelKey(this,0,true,page1->alwaysKey->text());

                if (opts->exec()==QDialog::Accepted) {
                        page1->alwaysKey->setText(opts->getkeyID());
                } else {
                        delete opts;
                        return;
                }
                delete opts;
}

void kgpgOptions::slotChangeHome()
{
QString gpgHome=KFileDialog::getExistingDirectory(page4->gpg_home_path->text(),this,i18n("New GnuPG Home Location"));
if (gpgHome.isEmpty()) return;
if (!gpgHome.endsWith("/")) gpgHome.append("/");
	QString confPath="options";
        if (!QFile(gpgHome+confPath).exists()) {
                confPath="gpg.conf";
		if (!QFile(gpgHome+confPath).exists())
		{
		if (KMessageBox::questionYesNo(this,i18n("No configuration file was found in the selected location.\nDo you want to create it now ?\n\nWithout configuration file, neither KGpg nor Gnupg will work properly."),i18n("No Configuration File Found"),i18n("Create"),i18n("Ignore"))==KMessageBox::Yes) //////////   Try to create config File by running gpg once
		{
		KProcIO *p=new KProcIO();
        	*p<<"gpg"<<"--homedir"<<gpgHome<<"--no-tty"<<"--list-secret-keys";
        	p->start(KProcess::Block);  ////  start gnupg so that it will create a config file
		confPath="gpg.conf";
		QFile confFile(gpgHome+confPath);
		if (!confFile.open(IO_WriteOnly))
		{KMessageBox::sorry(this,i18n("Cannot create configuration file. Please check if destination media is mounted and if you have write access"));
		return;
		}
		else
		{
		QTextStream stream( &confFile );
		stream<<"#  Config file created by KGpg\n\n";
		confFile.close();
		}
		}
		else confPath=QString::null;
		}
		}
		page4->gpg_conf_path->setText(confPath);
		page4->gpg_home_path->setText(gpgHome);
}

void kgpgOptions::updateWidgets()
{
QString pubKeyring,privKeyring;

        gpgConfigPath = KGpgSettings::gpgConfigPath();
	page4->gpg_conf_path->setText(KURL(gpgConfigPath).fileName());
	page4->gpg_home_path->setText(KURL(gpgConfigPath).directory(false));

	pubKeyring=KgpgInterface::getGpgSetting("keyring", gpgConfigPath);
	if (pubKeyring!="")
	{
	page4->kcfg_PubKeyringUrl->setURL(pubKeyring);
	page4->kcfg_PubKeyring->setChecked(true);
	}
	else page4->kcfg_PubKeyring->setChecked(false);

	privKeyring=KgpgInterface::getGpgSetting("secret-keyring", gpgConfigPath);
	if (privKeyring!="")
	{
	page4->kcfg_PrivKeyringUrl->setURL(privKeyring);
	page4->kcfg_PrivKeyring->setChecked(true);
	}
	else page4->kcfg_PrivKeyring->setChecked(false);

	page4->kcfg_OnlyAdditional->setChecked(KgpgInterface::getGpgBoolSetting("no-default-keyring", gpgConfigPath));

	// fill some values from kgpg's config file

        useAgent = KgpgInterface::getGpgBoolSetting("use-agent", gpgConfigPath);
        defaultUseAgent = false;

        page1->alwaysKey->setText(KgpgInterface::getGpgMultiSetting("encrypt-to", gpgConfigPath));
	alwaysKeyID = page1->alwaysKey->text();

        page4->use_agent->setChecked( useAgent );

        keyServer = KgpgInterface::getGpgSetting("keyserver", gpgConfigPath);
        defaultKeyServer = "hkp://wwwkeys.pgp.net";

        if (keyServer.isEmpty())
		keyServer = defaultKeyServer;

	page6->ServerBox->clear();
	page6->ServerBox->insertStringList(serverList);


	kdDebug(2100)<<"Finishing options"<<endl;
}

void kgpgOptions::updateWidgetsDefault()
{

	page1->alwaysKey->clear();
	page1->kcfg_FileKey->clear();

        page4->use_agent->setChecked( defaultUseAgent );

	page4->gpg_conf_path->setText(defaultConfigPath);
	page4->gpg_home_path->setText(defaultHomePath);

	page4->kcfg_PubKeyringUrl->setURL(QString::null);
	page4->kcfg_PubKeyring->setChecked(false);
	page4->kcfg_PrivKeyringUrl->setURL(QString::null);
	page4->kcfg_PrivKeyring->setChecked(false);
	page4->kcfg_OnlyAdditional->setChecked(false);


	page6->ServerBox->clear();
	page6->ServerBox->insertStringList(QStringList::split(",",defaultServerList));

	kdDebug(2100)<<"Finishing default options"<<endl;
}

bool kgpgOptions::isDefault()
{
	if (!page1->alwaysKey->text().isEmpty())
		return false;

        if (page4->gpg_conf_path->text()!=defaultConfigPath)
		return false;

	if (page4->gpg_home_path->text()!=defaultHomePath)
		return false;

	if (page4->use_agent->isChecked() != defaultUseAgent)
		return false;

	QString currList;
	for (uint i=0;i<page6->ServerBox->count();i++)
	currList+=page6->ServerBox->text(i)+",";
	currList.truncate(currList.length()-1);
	if (currList!=defaultServerList) return false;

	return true;
}

bool kgpgOptions::hasChanged()
{

	if (page1->alwaysKey->text()!= alwaysKeyID)
		return true;

	if (page4->gpg_conf_path->text() != KURL(gpgConfigPath).fileName())
		return true;
	if (page4->gpg_home_path->text() != KURL(gpgConfigPath).directory(false))
		return true;

	if (page4->use_agent->isChecked() != useAgent)
		return true;

	QStringList currList;
	for (uint i=0;i<page6->ServerBox->count();i++)
	currList.append(page6->ServerBox->text(i));
	if (currList!=serverList) return true;

	return false;
}

void kgpgOptions::updateSettings()
{
        // Update config path first!

	KGpgSettings::setGpgConfigPath( page4->gpg_home_path->text()+page4->gpg_conf_path->text() );
	KGpgSettings::writeConfig();	//Don't forget to write the config file
	if (page4->gpg_home_path->text()!=KURL(gpgConfigPath).directory(false))
	{
	if (page4->gpg_home_path->text()!=defaultHomePath)
	setenv("GNUPGHOME", QFile::encodeName(page4->gpg_home_path->text()), 1);
	else setenv("GNUPGHOME","",1);
	emit homeChanged();
	gpgConfigPath = KGpgSettings::gpgConfigPath();
	}

	bool emitReload=false;

	if (page4->kcfg_OnlyAdditional->isChecked()!=KgpgInterface::getGpgBoolSetting("no-default-keyring", gpgConfigPath)) emitReload=true;
	KgpgInterface::setGpgBoolSetting("no-default-keyring",page4->kcfg_OnlyAdditional->isChecked(), gpgConfigPath);

	if (page4->kcfg_PubKeyring->isChecked())
	{
	if (page4->kcfg_PubKeyringUrl->url()!=KgpgInterface::getGpgSetting("keyring", gpgConfigPath)) emitReload=true;
	KgpgInterface::setGpgSetting("keyring",page4->kcfg_PubKeyringUrl->url(), gpgConfigPath);
	}
	else
	{
	if (KgpgInterface::getGpgSetting("keyring", gpgConfigPath)!="") emitReload=true;
	KgpgInterface::setGpgSetting("keyring",QString::null, gpgConfigPath);
	}

	if (page4->kcfg_PrivKeyring->isChecked())
	{
	if (page4->kcfg_PrivKeyringUrl->url()!=KgpgInterface::getGpgSetting("secret-keyring", gpgConfigPath)) emitReload=true;
	KgpgInterface::setGpgSetting("secret-keyring",page4->kcfg_PrivKeyringUrl->url(), gpgConfigPath);
	}
	else
	{
	if (KgpgInterface::getGpgSetting("secret-keyring", gpgConfigPath)!="") emitReload=true;
	KgpgInterface::setGpgSetting("secret-keyring",QString::null, gpgConfigPath);
	}

	emit changeFont(kfc->font());
        ///////////////  install service menus

        if (page7->kcfg_SignMenu->currentItem()==KGpgSettings::EnumSignMenu::AllFiles)
                slotInstallSign("all/allfiles");
        else
                slotRemoveMenu("signfile.desktop");
        if (page7->kcfg_DecryptMenu->currentItem()==KGpgSettings::EnumDecryptMenu::AllFiles)
                slotInstallDecrypt("all/allfiles");
        else if (page7->kcfg_DecryptMenu->currentItem()==KGpgSettings::EnumDecryptMenu::EncryptedFiles)
                slotInstallDecrypt("application/pgp-encrypted,application/pgp-signature,application/pgp-keys");
        else
                slotRemoveMenu("decryptfile.desktop");

	KgpgInterface::setGpgMultiSetting("encrypt-to",QStringList::split(" ",page1->alwaysKey->text()),KGpgSettings::gpgConfigPath());
	alwaysKeyID = page1->alwaysKey->text();

        useAgent = page4->use_agent->isChecked();

	if (useAgent)
	{
		KgpgInterface::setGpgBoolSetting("use-agent",true, KGpgSettings::gpgConfigPath());
		KgpgInterface::setGpgBoolSetting("no-use-agent",false, KGpgSettings::gpgConfigPath());
	}
	else
	{
	//	KgpgInterface::setGpgBoolSetting("no-use-agent",true, KGpgSettings::gpgConfigPath());
		KgpgInterface::setGpgBoolSetting("use-agent",false, KGpgSettings::gpgConfigPath());
	}

	//////////////////  save key servers


	QString currList;
	serverList=QStringList ();
	for (uint i=0;i<page6->ServerBox->count();i++)
	{
	QString currItem=page6->ServerBox->text(i);
	if (currItem.find(" ")!=-1) // it is the default keyserver
	keyServer=currItem.section(" ",0,0);
	else
	{
	serverList.append(currItem);
	}
	}

	KgpgInterface::setGpgSetting("keyserver",keyServer, KGpgSettings::gpgConfigPath());
	serverList.prepend(keyServer+" "+i18n("(Default)"));
	currList=serverList.join(",");

	if (keyGood!=page3->kcfg_ColorGood->color()) 
	emit refreshTrust(GoodColor,page3->kcfg_ColorGood->color());
	if (keyBad!=page3->kcfg_ColorBad->color()) emit refreshTrust(BadColor,page3->kcfg_ColorBad->color());
	if (keyUnknown!=page3->kcfg_ColorUnknown->color()) emit refreshTrust(UnknownColor,page3->kcfg_ColorUnknown->color());
	if (keyRev!=page3->kcfg_ColorRev->color()) emit refreshTrust(RevColor,page3->kcfg_ColorRev->color());


//	KGpgSettings::writeConfig();
	config->setGroup("Servers");
	config->writeEntry("Server_List",currList);
	emit settingsUpdated();
	if (emitReload) emit reloadKeyList();
}


void kgpgOptions::slotInstallSign(QString mimetype)
{

        QString path=locateLocal("data","konqueror/servicemenus/signfile.desktop");
        KDesktopFile configl2(path, false);
        if (configl2.isImmutable() ==false) {
                configl2.setGroup("Desktop Entry");
                configl2.writeEntry("ServiceTypes", mimetype);
                configl2.writeEntry("Actions", "sign");
                configl2.setGroup("Desktop Action sign");
                configl2.writeEntry("Name",i18n("Sign File"));
                //configl2.writeEntry("Icon", "sign_file");
                configl2.writeEntry("Exec","kgpg -S %F");
                //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
        }
}

void kgpgOptions::slotInstallDecrypt(QString mimetype)
{

        QString path=locateLocal("data","konqueror/servicemenus/decryptfile.desktop");
        KDesktopFile configl2(path, false);
        if (configl2.isImmutable() ==false) {
                configl2.setGroup("Desktop Entry");
                configl2.writeEntry("ServiceTypes", mimetype);
                configl2.writeEntry("Actions", "decrypt");
                configl2.setGroup("Desktop Action decrypt");
                configl2.writeEntry("Name",i18n("Decrypt File"));
                //configl2.writeEntry("Icon", "decrypt_file");
                configl2.writeEntry("Exec","kgpg %U");
                //KMessageBox::information(this,i18n("Decrypt file option is now added in Konqueror's menu."));
        }
}


void kgpgOptions::slotRemoveMenu(QString menu)
{

        QString path=locateLocal("data","konqueror/servicemenus/"+menu);
        QFile qfile(path);
        if (qfile.exists())
                qfile.remove();
        {
                //if (!qfile.remove()) KMessageBox::sorry(this,i18n("Cannot remove service menu. Check permissions"));
                //else KMessageBox::information(this,i18n("Service menu 'Decrypt File' has been removed."));
        }
        //else KMessageBox::sorry(this,i18n("No service menu found"));

}

QString kgpgOptions::namecode(QString kid)
{

        for ( uint counter = 0; counter<names.count(); counter++ )
                if (QString(ids[counter].right(8))==kid)
                        return names[counter];

        return QString::null;
}


QString kgpgOptions::idcode(QString kname)
{
        for ( uint counter = 0; counter<names.count(); counter++ )
                if (names[counter]==kname)
                        return QString(ids[counter].right(8));
        return QString::null;
}


void kgpgOptions::listkey()
{

        ////////   update display of keys in main management window
        FILE *fp;
        QString tst,name,trustedvals="idre-",issec;
        int counter=0;
        char line[300];

	FILE *fp2;

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=line;
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);


        fp = popen("gpg --no-tty --with-colon --list-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
                        name=KgpgInterface::checkForUtf8(tst.section(':',9,9));
                        if ((!name.isEmpty()) && (trustedvals.find(tst.section(':',1,1))==-1)) {
                                counter++;
                                //name=name.section('<',-1,-1);
                                //  name=name.section('>',0,0);
                                names+=name;
                                ids+=tst.section(':',4,4);
                                if (tst.section(':',4,4).right(8)==alwaysKeyID)
                                        alwaysKeyName=tst.section(':',4,4).right(8)+":"+name;
				if (issec.find(tst.section(':',4,4).right(8),0,FALSE)!=-1)
				{
//***                           page1->file_key->insertItem(pixkeyDouble,tst.section(':',4,4).right(8)+":"+name);
//***                           page1->always_key->insertItem(pixkeyDouble,tst.section(':',4,4).right(8)+":"+name);
				}
				else
				{
//***                           page1->file_key->insertItem(pixkeySingle,tst.section(':',4,4).right(8)+":"+name);
//***                           page1->always_key->insertItem(pixkeySingle,tst.section(':',4,4).right(8)+":"+name);
				}
                        }
                }
        }
        pclose(fp);
        if (counter==0) {
                ids+="0";
//***                           page1->file_key->insertItem(i18n("none"));
//***                           page1->always_key->insertItem(i18n("none"));
        }
}

void kgpgOptions::slotAddKeyServer()
{
QString newServer=KInputDialog::getText(i18n("Add New Key Server"),i18n("Server URL:"));
if (!newServer.isEmpty())
page6->ServerBox->insertItem(newServer.stripWhiteSpace());
page6->ServerBox->setSelected(page6->ServerBox->findItem(newServer.stripWhiteSpace()),true);
}

void kgpgOptions::slotDelKeyServer()
{
bool defaultDeleted=false;
if (page6->ServerBox->currentText().find(" ")!=-1) defaultDeleted=true;
page6->ServerBox->removeItem(page6->ServerBox->currentItem());
page6->ServerBox->setSelected(0,true);
if (defaultDeleted) page6->ServerBox->changeItem(page6->ServerBox->currentText().section(" ",0,0)+" "+i18n("(Default)"),0);
}

void kgpgOptions::slotDefaultKeyServer()
{
uint curr=page6->ServerBox->currentItem();
page6->ServerBox->changeItem(page6->ServerBox->currentText ().section(" ",0,0)+" "+i18n("(Default)"),curr);

for (uint i=0;i<page6->ServerBox->count();i++)
{
if (i!=curr)
page6->ServerBox->changeItem(page6->ServerBox->text(i).section(" ",0,0),i);
}
page6->ServerBox->setSelected(curr,true);
}

#include "kgpgoptions.moc"

