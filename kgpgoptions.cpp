/***************************************************************************
                          kgpgoptions.cpp  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
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

#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>

#include <kmessagebox.h>
#include <klineedit.h>
#include <ktoolbar.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <kdialogbase.h>

#include <kdesktopfile.h>
#include <kmimetype.h>
#include <kstandarddirs.h>


#include "kgpgoptions.h"
#include "kgpgsettings.h"
#include "kgpg.h"


///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name)
 : KConfigDialog( parent, name, KGpgSettings::self())
{
kdDebug()<<"Adding pages"<<endl;
        page1=new Encryption();
        page2=new Decryption();
        page3=new UI();
        page4=new GPGConf();
        addPage(page1, i18n("Encryption"), "encrypted");
        addPage(page2, i18n("Decryption"), "decrypted");
        addPage(page3, i18n("User Interface"), "misc");
        addPage(page4, i18n("GPG Settings"), "kgpg");

        // The following widgets are managed manually.
        connect(page1->encrypt_to_always, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
        connect(page1->file_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
        connect(page1->always_key, SIGNAL(activated(int)), this, SLOT(updateButtons()));
        connect(page4->gpg_config_path, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
        connect(page4->use_agent, SIGNAL(toggled(bool)), this, SLOT(updateButtons()));
        connect(page4->key_server1, SIGNAL(textChanged(const QString&)), this, SLOT(updateButtons()));
}


kgpgOptions::~kgpgOptions()
{}

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
        page4->gpg_config_path->setURL(gpgConfigPath);

        useAgent = KgpgInterface::getGpgBoolSetting("use-agent", KGpgSettings::gpgConfigPath());
        defaultUseAgent = false;

        page4->use_agent->setChecked( useAgent );

        keyServer = KgpgInterface::getGpgSetting("keyserver", KGpgSettings::gpgConfigPath());
        defaultKeyServer = "hkp://wwwkeys.pgp.net";
        
        if (keyServer.isEmpty())
		keyServer = defaultKeyServer;

        page4->key_server1->setText(keyServer);

	kdDebug()<<"Finishing options"<<endl;
}

void kgpgOptions::updateWidgetsDefault()
{
        page1->encrypt_to_always->setChecked( defaultEncryptToAlways );

        // Note that we don't set a default for gpg_config_path

        page4->use_agent->setChecked( defaultUseAgent );

	page4->key_server1->setText( defaultKeyServer );

	kdDebug()<<"Finishing default options"<<endl;
}

bool kgpgOptions::isDefault()
{
	if (page1->encrypt_to_always->isChecked() != defaultEncryptToAlways)
		return false;

        // Note that we don't check gpg_config_path

	if (page4->use_agent->isChecked() != defaultUseAgent)
		return false;
	   
	if (page4->key_server1->text().stripWhiteSpace() != defaultKeyServer)
		return false;

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

	if (page4->gpg_config_path->url() != gpgConfigPath)
		return true;
	   
	if (page4->use_agent->isChecked() != useAgent)
		return true;

	if (page4->key_server1->text().stripWhiteSpace() != keyServer)
		return true;

	return false;
}

void kgpgOptions::updateSettings()
{
        // Update config path first!
	KGpgSettings::setGpgConfigPath( page4->gpg_config_path->url() );

        ////////////  save selected keys for file encryption & always encrypt with
        if (page1->kcfg_EncryptFilesTo->isChecked())
		fileEncryptionKey = page1->file_key->currentText();
	else
		fileEncryptionKey = "";
        KGpgSettings::setFileEncryptionKey( fileEncryptionKey );

	encryptToAlways = page1->encrypt_to_always->isChecked();

        if (encryptToAlways)
        	alwaysKeyID = page1->always_key->currentText().section(':',0,0);
        else
        	alwaysKeyID = "";
	KgpgInterface::setGpgSetting("encrypt-to",alwaysKeyID, KGpgSettings::gpgConfigPath());

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

        ////////////  save the first key server into GnuPG's configuration file
        keyServer = page4->key_server1->text().stripWhiteSpace();
        if (!keyServer.isEmpty())
                KgpgInterface::setGpgSetting("keyserver", keyServer, KGpgSettings::gpgConfigPath());


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
	KGpgSettings::writeConfig();
	
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
                configl2.writeEntry("Exec","kgpg -S %U");
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
        QString tst,name,trustedvals="idre-";
        int counter=0;
        char line[130];

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
                                page1->file_key->insertItem(tst.section(':',4,4).right(8)+":"+name);
                                page1->always_key->insertItem(tst.section(':',4,4).right(8)+":"+name);
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

#include "kgpgoptions.moc"
