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
#include "kgpg.h"


///////////////////////   main window

kgpgOptions::kgpgOptions(QWidget *parent, const char *name):KAutoConfigDialog( parent, name)
{
        config=kapp->config();

        config->setGroup("User Interface");


        page1=new Encryption();
        page2=new Decryption();
        page3=new UI();
        page4=new GPGConf();
        addPage(page1, i18n("Encryption"), "Encryption", "encrypted");
        addPage(page2, i18n("Decryption"), "Decryption", "decrypted");
        addPage(page3, i18n("User Interface"), "User Interface", "misc");
        addPage(page4, i18n("GPG Settings"), "GPG Settings", "gpg");



        config->setGroup("GPG Settings");
        alwaysKeyID=KgpgInterface::getGpgSetting("encrypt-to",config->readPathEntry("gpg_config_path"));
	config->writeEntry("use_agent",KgpgInterface::getGpgBoolSetting("use-agent",config->readPathEntry("gpg_config_path")));

	config->setGroup("Encryption");
        if (!alwaysKeyID.isEmpty())
                config->writeEntry("encrypt_to_always",true);
        else
                config->writeEntry("encrypt_to_always",false);
        listkey();


        QString key=config->readEntry("file key");
        if (key!=NULL) {
                page1->file_key->setCurrentItem(key);
                config->writeEntry("file_key",page1->file_key->currentItem());
        }


        if (alwaysKeyID!=NULL) {
                page1->always_key->setCurrentItem(alwaysKeyName);
                config->writeEntry("always_key",page1->always_key->currentItem());
        }
        //////////////////////  check if gpg's keyserver is the same as KGpg's first server. Otherwise, syncro
        config->setGroup("GPG Settings");
        QString optionsServer=KgpgInterface::getGpgSetting("keyserver",config->readPathEntry("gpg_config_path"));
        if (!optionsServer.isEmpty())
                config->writeEntry("key_server1",optionsServer);

        connect(this, SIGNAL(settingsChanged()), this, SLOT(readSettings()));
}


kgpgOptions::~kgpgOptions()
{}


void kgpgOptions::readSettings()
{

        ////////////  save selected keys for file encryption & always encrypt with
        config->setGroup("Encryption");
        config->writeEntry("file key",page1->file_key->currentText());
        if (page1->encrypt_to_always->isChecked()) {
                config->writeEntry("always key",page1->always_key->currentText().section(':',0,0));
                KgpgInterface::setGpgSetting("encrypt-to",page1->always_key->currentText().section(':',0,0),page4->gpg_config_path->url());
        } else
                KgpgInterface::setGpgSetting("encrypt-to","",page4->gpg_config_path->url());

        ////////////  save the first key server into GnuPG's configuration file
        if (!page4->key_server1->text().isEmpty())
                KgpgInterface::setGpgSetting("keyserver",page4->key_server1->text().stripWhiteSpace(),page4->gpg_config_path->url());

        ///////////////  install service menus

        if (page3->sign_menu->currentItem()==1)
                slotInstallSign("allfiles");
        else
                slotRemoveMenu("signfile.desktop");
        if (page3->decrypt_menu->currentItem()==1)
                slotInstallDecrypt("allfiles");
        else if (page3->decrypt_menu->currentItem()==2)
                slotInstallDecrypt("application/pgp-encrypted,application/pgp-signature,application/pgp-keys");
        else
                slotRemoveMenu("decryptfile.desktop");

if (page4->use_agent->isChecked())
	{
	KgpgInterface::setGpgBoolSetting("use-agent",true,page4->gpg_config_path->url());
	KgpgInterface::setGpgBoolSetting("no-use-agent",false,page4->gpg_config_path->url());
	}
else
	{
//	KgpgInterface::setGpgBoolSetting("no-use-agent",true,page4->gpg_config_path->url());
	KgpgInterface::setGpgBoolSetting("use-agent",false,page4->gpg_config_path->url());
	}

        emit updateSettings();
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
                if (tst.find("pub",0,FALSE)!=-1) {
                        name=tst.section(':',9,9);
                        if ((name!="") && (trustedvals.find(tst.section(':',1,1))==-1)) {
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
