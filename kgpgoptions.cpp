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


#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qfile.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kprocio.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>
#include <qfont.h>
#include <kaction.h>
#include <qcombobox.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <ktoolbar.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kdialogbase.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kfontdialog.h>
#include <kdesktopfile.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <stdlib.h>
#include <kdebug.h>

#include "kgpgoptions.h"
#include "kgpgsettings.h"

#include "conf_decryption.h"
#include "conf_encryption.h"
#include "conf_gpg.h"
#include "conf_servers.h"
#include "conf_ui.h"
#include "conf_colors.h"

///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name)
 : KConfigDialog( parent, name, KGpgSettings::self())
{
	ks=new KSimpleConfig ("kgpgrc");
	ks->setGroup("Editor");
	startFont=ks->readFontEntry("Editor_Font");

	defaultServerList="hkp://wwwkeys.eu.pgp.net ";
	defaultServerList+=i18n("(Default)");
	defaultServerList+=",hkp://search.keyserver.net,hkp://wwwkeys.pgp.net,hkp://pgp.dtype.org,hkp://wwwkeys.us.pgp.net";
	ks->setGroup("Servers");
	serverList=QStringList::split (",",ks->readEntry("Server_List",defaultServerList));
	keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());
	if (!keyServer.isEmpty()) serverList.prepend(keyServer+" "+i18n("(Default)"));
	
	defaultHomePath=QDir::homeDirPath()+"/.gnupg/";
	//defaultConfigPath=defaultHomePath+"options";
        if (QFile(defaultHomePath+"options").exists()) 
	defaultConfigPath="options";
	else
	{
                //defaultConfigPath=QDir::homeDirPath()+"/.gnupg/gpg.conf";
                if (QFile(defaultHomePath+"gpg.conf").exists()) defaultConfigPath="gpg.conf";
		else defaultConfigPath=QString::null;
		}
	
kdDebug(2100)<<"Adding pages"<<endl;
        page1=new Encryption();
        page2=new Decryption();
        page3=new UI();
        page4=new GPGConf();
	page6=new ServerConf();
	page7=new KFontChooser();
	page8=new ColorsConf();
	pixkeySingle=KGlobal::iconLoader()->loadIcon("kgpg_key1",KIcon::Small,20);
	pixkeyDouble=KGlobal::iconLoader()->loadIcon("kgpg_key2",KIcon::Small,20);
        addPage(page1, i18n("Encryption"), "encrypted");
        addPage(page2, i18n("Decryption"), "decrypted");
        addPage(page3, i18n("User Interface"), "misc");
        addPage(page4, i18n("GnuPG Settings"), "kgpg");
//	addPage(page5, i18n("GPG Settings2"), "kgpg");
	addPage(page6, i18n("Key Servers"), "network");
	addPage(page7, i18n("Editor Font"), "fonts");  //,QString::null,false);
	page7->setFont(startFont);
	addPage(page8, i18n("Key Colors"), "colorize");
	

        // The following widgets are managed manually.
        connect(page1->encrypt_to_always, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
        connect(page1->file_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
        connect(page1->always_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
	connect(page4->gpg_conf_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
	connect(page4->gpg_home_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
        connect(page4->use_agent, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
	connect(page4->changeHome, SIGNAL(clicked()), this, SLOT(slotChangeHome()));
	connect(page6->server_add, SIGNAL(clicked()), this, SLOT(slotAddKeyServer()));
	connect(page6->server_del, SIGNAL(clicked()), this, SLOT(slotDelKeyServer()));
	connect(page6->server_default, SIGNAL(clicked()), this, SLOT(slotDefaultKeyServer()));
	connect(page6->ServerBox, SIGNAL(currentChanged ( QListBoxItem *)), this, SLOT(updateButtons()));
	connect(page7, SIGNAL(fontSelected(const QFont &)), this, SLOT(updateButtons()));
}


kgpgOptions::~kgpgOptions()
{}

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
        alwaysKeyID=KgpgInterface::getGpgSetting("encrypt-to", KGpgSettings::gpgConfigPath());

        encryptToAlways = !alwaysKeyID.isEmpty();
        defaultEncryptToAlways = false;
       
        page1->encrypt_to_always->setChecked(encryptToAlways);

        listkey();
        fileEncryptionKey = KGpgSettings::fileEncryptionKey();
        if (!fileEncryptionKey.isEmpty()) {
                page1->file_key->setCurrentItem(fileEncryptionKey);
        }

        if (!alwaysKeyName.isEmpty()) {
                page1->always_key->setCurrentItem(alwaysKeyName);
        }
        
        gpgConfigPath = KGpgSettings::gpgConfigPath();
	page4->gpg_conf_path->setText(KURL(gpgConfigPath).fileName());
	page4->gpg_home_path->setText(KURL(gpgConfigPath).directory(false));

        useAgent = KgpgInterface::getGpgBoolSetting("use-agent", KGpgSettings::gpgConfigPath());
        defaultUseAgent = false;

        page4->use_agent->setChecked( useAgent );

        keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());
        defaultKeyServer = "hkp://wwwkeys.pgp.net";
        
        if (keyServer.isEmpty())
		keyServer = defaultKeyServer;

        page7->setFont(startFont);
	
	page6->ServerBox->clear();
	page6->ServerBox->insertStringList(serverList);
	
	
	kdDebug(2100)<<"Finishing options"<<endl;
}

void kgpgOptions::updateWidgetsDefault()
{
        page1->encrypt_to_always->setChecked( defaultEncryptToAlways );

 

        page4->use_agent->setChecked( defaultUseAgent );
	
	page7->setFont(QFont());
	
	
	page4->gpg_conf_path->setText(defaultConfigPath);
	page4->gpg_home_path->setText(defaultHomePath);
	
	page6->ServerBox->clear();
	page6->ServerBox->insertStringList(QStringList::split(",",defaultServerList));

	kdDebug(2100)<<"Finishing default options"<<endl;
}

bool kgpgOptions::isDefault()
{
	if (page1->encrypt_to_always->isChecked() != defaultEncryptToAlways)
		return false;

        if (page4->gpg_conf_path->text()!=defaultConfigPath)
		return false;
		
	if (page4->gpg_home_path->text()!=defaultHomePath)
		return false;

	if (page4->use_agent->isChecked() != defaultUseAgent)
		return false;
	   
	if (page7->font()!=QFont())
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
	if (page1->kcfg_EncryptFilesTo->isChecked() &&
	    (page1->file_key->currentText() != fileEncryptionKey))
		return true;

	if (page1->encrypt_to_always->isChecked() != encryptToAlways)
		return true;

	if (page1->encrypt_to_always->isChecked() &&
	    (page1->always_key->currentText().section(':',0,0) != alwaysKeyID))
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

	if (page7->font()!=startFont)
		return true;
	return false;
}

void kgpgOptions::updateSettings()
{
        // Update config path first!
	KGpgSettings::setGpgConfigPath( page4->gpg_home_path->text()+page4->gpg_conf_path->text() );
	if (page4->gpg_home_path->text()!=KURL(gpgConfigPath).directory(false))
	{

	if (page4->gpg_home_path->text()!=defaultHomePath)
	setenv("GNUPGHOME",page4->gpg_home_path->text().ascii(),1);
	else setenv("GNUPGHOME","",1);
	emit homeChanged();
	gpgConfigPath = KGpgSettings::gpgConfigPath();
	}
	
        ////////////  save selected keys for file encryption & always encrypt with
        if (page1->kcfg_EncryptFilesTo->isChecked())
		fileEncryptionKey = page1->file_key->currentText();
	else
		fileEncryptionKey = QString::null;
	if (fileEncryptionKey!=KGpgSettings::fileEncryptionKey())
        KGpgSettings::setFileEncryptionKey( fileEncryptionKey );

	encryptToAlways = page1->encrypt_to_always->isChecked();

        if (encryptToAlways)
        	alwaysKeyID = page1->always_key->currentText().section(':',0,0);
        else
        	alwaysKeyID = QString::null;
	KgpgInterface::setGpgSetting("encrypt-to",alwaysKeyID, KGpgSettings::gpgConfigPath());
	
	//////////////////  save key servers
	
	if (page7->font()!=startFont)
	{
	emit changeFont(page7->font());
	startFont=page7->font();
	ks->setGroup("Editor");
	ks->writeEntry("Editor_Font",startFont);
	}
	

        ///////////////  install service menus

        if (page3->kcfg_SignMenu->currentItem()==KGpgSettings::EnumSignMenu::AllFiles)
                slotInstallSign("all/allfiles");
        else
                slotRemoveMenu("signfile.desktop");
        if (page3->kcfg_DecryptMenu->currentItem()==KGpgSettings::EnumDecryptMenu::AllFiles)
                slotInstallDecrypt("all/allfiles");
        else if (page3->kcfg_DecryptMenu->currentItem()==KGpgSettings::EnumDecryptMenu::EncryptedFiles)
                slotInstallDecrypt("application/pgp-encrypted,application/pgp-signature,application/pgp-keys");
        else
                slotRemoveMenu("decryptfile.desktop");

	
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
	currList=serverList.join(",");
	ks->setGroup("Servers");
	ks->writeEntry("Server_List",currList);
	KgpgInterface::setGpgSetting("keyserver",keyServer, KGpgSettings::gpgConfigPath());
	serverList.prepend(keyServer+" "+i18n("(Default)"));
	

	KGpgSettings::writeConfig();
	//ks->sync();
	emit settingsUpdated();
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
				page1->file_key->insertItem(pixkeyDouble,tst.section(':',4,4).right(8)+":"+name);
                                page1->always_key->insertItem(pixkeyDouble,tst.section(':',4,4).right(8)+":"+name);
				}
				else
				{
                                page1->file_key->insertItem(pixkeySingle,tst.section(':',4,4).right(8)+":"+name);
                                page1->always_key->insertItem(pixkeySingle,tst.section(':',4,4).right(8)+":"+name);
				}
                        }
                }
        }
        pclose(fp);
        if (counter==0) {
                ids+="0";
                page1->file_key->insertItem(i18n("none"));
                page1->always_key->insertItem(i18n("none"));
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
