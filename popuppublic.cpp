/***************************************************************************
                          popuppublic.cpp  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by
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

////////////////////////////////////////////////////////   code  for choosing a public key from a list for encryption
#include <qpushbutton.h>
#include <qptrlist.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qiconset.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <klistview.h>
#include <kprocess.h>
#include <kprocio.h>
#include <qhbuttongroup.h>
#include <klocale.h>

#include "popuppublic.h"
#include "kgpgview.h"
#include "kgpg.h"

/////////////////   klistviewitem special

class UpdateViewItem2 : public KListViewItem
{
public:
        UpdateViewItem2(QListView *parent, QString name,QString mail,QString id,bool isDefault);
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
	virtual QString key(int c,bool ) const;
	bool def;
};

UpdateViewItem2::UpdateViewItem2(QListView *parent, QString name,QString mail,QString id,bool isDefault)
                : KListViewItem(parent)
{
def=isDefault;
        setText(0,name);
	setText(1,mail);
	setText(2,id);
}


void UpdateViewItem2::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        if ((def) && (column<2)) {
                QFont font(p->font());
                font.setBold(true);
                p->setFont(font);
        }
        KListViewItem::paintCell(p, cg, column, width, alignment);
}

QString UpdateViewItem2 :: key(int c,bool ) const
{
        return text(c).lower();
}

///////////////  main view

popupPublic::popupPublic(QWidget *parent, const char *name,QString sfile,bool filemode): 
KDialogBase( Plain, i18n("Select Public Key"), Details | Ok | Cancel, Ok, parent, name,true)
{

	QWidget *page = plainPage();
	QVBoxLayout *vbox=new QVBoxLayout(page,0,spacingHint());
	vbox->setAutoAdd(true);
	
	setButtonText(KDialogBase::Details,i18n("Options"));

        config=kapp->config();

        //pgpcomp=config->readBoolEntry("PGP compatibility",false);
        config->setGroup("Encryption");
	defaultKey=config->readEntry("default key");
        allowcustom=config->readBoolEntry("allow_custom_option",false);
        if (allowcustom)
                customOptions=config->readEntry("custom_option");
	config->setGroup("GPG Settings");
	keyGroups=config->readEntry("Groups");

        KIconLoader *loader = KGlobal::iconLoader();

        keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
        keySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
	keyGroup=loader->loadIcon("kgpg_key3",KIcon::Small,20);
	
        if (filemode) setCaption(i18n("Select Public Key for %1").arg(sfile));

        fmode=filemode;

        keysList = new KListView( page );
	 keysList->addColumn(i18n("Name"));
	 keysList->addColumn(i18n("Email"));
	 keysList->addColumn(i18n("ID"));

        keysList->setRootIsDecorated(false);
        page->setMinimumSize(540,200);
        keysList->setShowSortIndicator(true);
        keysList->setFullWidth(true);
	keysList->setAllColumnsShowFocus(true);
        keysList->setSelectionModeExt(KListView::Extended);
	keysList->setColumnWidthMode(0,QListView::Manual);
	keysList->setColumnWidthMode(1,QListView::Manual);
	keysList->setColumnWidth(0,210);
	keysList->setColumnWidth(1,210);

        boutonboxoptions=new QButtonGroup(5,Qt::Vertical ,page,0);

        CBarmor=new QCheckBox(i18n("ASCII armored encryption"),boutonboxoptions);
        CBuntrusted=new QCheckBox(i18n("Allow encryption with untrusted keys"),boutonboxoptions);
        CBhideid=new QCheckBox(i18n("Hide user id"),boutonboxoptions);
        setDetailsWidget(boutonboxoptions);
        QWhatsThis::add
                (keysList,i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
        QWhatsThis::add
                (CBarmor,i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));
        QWhatsThis::add
                (CBhideid,i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                                "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because "
                                "all available secret keys are tried."));
        QWhatsThis::add
                (CBuntrusted,i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                                  "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                                  "box enables you to use any key, even if it has not be signed."));

        if (filemode) {
                CBshred=new QCheckBox(i18n("Shred source file"),boutonboxoptions);
                QWhatsThis::add
                        (CBshred,i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));

                CBsymmetric=new QCheckBox(i18n("Symmetrical encryption"),boutonboxoptions);
                QWhatsThis::add
                        (CBsymmetric,i18n("<b>Symmetrical encryption</b>: encryption doesn't use keys. You just need to give a password "
                                          "to encrypt/decrypt the file"));
                QObject::connect(CBsymmetric,SIGNAL(toggled(bool)),this,SLOT(isSymetric(bool)));
        }

        config->setGroup("Encryption");
	
	CBarmor->setChecked(config->readBoolEntry("Ascii_armor",true));
	CBuntrusted->setChecked(config->readBoolEntry("Allow_untrusted_keys",false));
	CBhideid->setChecked(config->readBoolEntry("Hide_user_id",false));
	if (filemode) CBshred->setChecked(config->readBoolEntry("shred_source",false));
        
        if (allowcustom) {
                QHButtonGroup *bGroup = new QHButtonGroup(page);
                //bGroup->setFrameStyle(QFrame::NoFrame);

                (void) new QLabel(i18n("Custom option:"),bGroup);
                KLineEdit *optiontxt=new KLineEdit(bGroup);
                optiontxt->setText(customOptions);
                QWhatsThis::add
                        (optiontxt,i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));
                QObject::connect(optiontxt,SIGNAL(textChanged ( const QString & )),this,SLOT(customOpts(const QString & )));
        }
        QObject::connect(keysList,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(crypte()));
	QObject::connect(this,SIGNAL(okClicked()),this,SLOT(crypte()));
        QObject::connect(CBuntrusted,SIGNAL(toggled(bool)),this,SLOT(refresh(bool)));
	
        char line[200]="\0";
        FILE *fp2;
        seclist=QString::null;

        fp2 = popen("gpg --no-secmem-warning --no-tty --list-secret-keys ", "r");
        while ( fgets( line, sizeof(line), fp2))
                seclist+=line;
        pclose(fp2);

        trusted=CBuntrusted->isChecked();
	
        refreshkeys();
	setMinimumSize(550,200);
	updateGeometry();
	keysList->setFocus();
	show();
}

popupPublic::~popupPublic()
{}

void popupPublic::slotAccept()
{
accept(); 
}

void popupPublic::enable()
{
        QListViewItem *current = keysList->firstChild();
        if (current==NULL)
                return;
        current->setVisible(true);
        while ( current->nextSibling() ) {
                current = current->nextSibling();
                current->setVisible(true);
        }
}

void popupPublic::sort()
{
        bool reselect=false;
        QListViewItem *current = keysList->firstChild();
        if (current==NULL)
                return;

	if ((untrustedList.find(current->text(2))!=untrustedList.end()) && (!current->text(2).isEmpty())){
                if (current->isSelected()) {
                        current->setSelected(false);
                        reselect=true;
                }
                current->setVisible(false);
		}

        while ( current->nextSibling() ) {
                current = current->nextSibling();
                if ((untrustedList.find(current->text(2))!=untrustedList.end()) && (!current->text(2).isEmpty())) {
                if (current->isSelected()) {
                        current->setSelected(false);
                        reselect=true;
                }
                current->setVisible(false);
		}
        }

        if (reselect) {
                QListViewItem *firstvisible;
                firstvisible=keysList->firstChild();
                while (firstvisible->isVisible()!=true) {
                        firstvisible=firstvisible->nextSibling();
                        if (firstvisible==NULL)
                                return;
                }
                keysList->setSelected(firstvisible,true);
                keysList->setCurrentItem(firstvisible);
		keysList->ensureItemVisible(firstvisible);
        }
}

void popupPublic::isSymetric(bool state)
{
        keysList->setEnabled(!state);
        CBuntrusted->setEnabled(!state);
        CBhideid->setEnabled(!state);
}


void popupPublic::customOpts(const QString &str)
{
        customOptions=str;
}

void popupPublic::refresh(bool state)
{
        if (state)
                enable();
        else
                sort();
}

void popupPublic::refreshkeys()
{
keysList->clear();
if (!keyGroups.isEmpty())
{
QStringList groups=QStringList::split(",",keyGroups);
	for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
	if (!QString(*it).isEmpty())
	{
			UpdateViewItem2 *item=new UpdateViewItem2(keysList,QString(*it),QString::null,QString::null,false);
			item->setPixmap(0,keyGroup);
	}
}
        KProcIO *encid=new KProcIO();
        *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
        /////////  when process ends, update dialog infos
        QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotpreselect()));
        QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
        encid->start(KProcess::NotifyOnExit,true);
}

void popupPublic::slotpreselect()
{
QListViewItem *it;
        if (fmode) it=keysList->findItem("0x"+defaultKey,2);        
        else {
                it=keysList->firstChild();
                if (it==NULL)
                        return;
                while (!it->isVisible()) {
                        it=it->nextSibling();
                        if (it==NULL)
                                return;
                }
        }
	keysList->setSelected(it,true);
	keysList->setCurrentItem(it);
	keysList->ensureItemVisible(it);
        if (!trusted)
              sort();
}


void popupPublic::slotprocread(KProcIO *p)
{
        ///////////////////////////////////////////////////////////////// extract  encryption keys
        bool dead;
        QString tst,keyname,keymail;

        while (p->readln(tst)!=-1) {
                if (tst.startsWith("pub")) {
			QStringList keyString=QStringList::split(":",tst,true);
                        dead=false;
                        const QString trust=keyString[1];
                        QString val=keyString[6];
                        QString id=QString("0x"+keyString[4].right(8));
                        if (val.isEmpty())
                                val=i18n("Unlimited");
                        QString tr;
                        switch( trust[0] ) {
                        case 'o':
				untrustedList<<id;
                                break;
                        case 'i':
                                dead=true;
                                break;
                        case 'd':
                                dead=true;
                                break;
                        case 'r':
                                dead=true;
                                break;
                        case 'e':
                                dead=true;
                                break;
                        case 'q':
                                untrustedList<<id;
                                break;
                        case 'n':
                                untrustedList<<id;
                                break;
                        case 'm':
                                untrustedList<<id;
                                break;
                        case 'f':
                                break;
                        case 'u':
                                break;
                        default:
				untrustedList<<id;
                                break;
                        }
			if (keyString[11].find('D')!=-1) dead=true;
                        tst=keyString[9];
			if (tst.find("<")!=-1) {
                keymail=tst.section('<',-1,-1);
                keymail.truncate(keymail.length()-1);
                keyname=tst.section('<',0,0);
                if (keyname.find("(")!=-1)
                        keyname=keyname.section('(',0,0);
        } else {
                keymail=QString::null;
                keyname=tst.section('(',0,0);
        }

	keyname=KgpgInterface::checkForUtf8(keyname);

                        if ((!dead) && (!tst.isEmpty())) {
				bool isDefaultKey=false;
                                if (id.right(8)==defaultKey) isDefaultKey=true;
                                        UpdateViewItem2 *item=new UpdateViewItem2(keysList,keyname,keymail,id,isDefaultKey);
					//KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, validity: %3").arg(id).arg(tr).arg(val));
                                        //sub->setSelectable(false);
                                        if (seclist.find(tst,0,FALSE)!=-1)
                                                item->setPixmap(0,keyPair);
                                        else
                                                item->setPixmap(0,keySingle);
                        }
                }
        }
}


void popupPublic::crypte()
{
        //////   emit selected data

        QStringList selectedKeys;
	QString userid;
        QPtrList<QListViewItem> list=keysList->selectedItems();

        for ( uint i = 0; i < list.count(); ++i )
                if ( list.at(i) ) {
			if (!list.at(i)->text(2).isEmpty()) selectedKeys<<list.at(i)->text(2);
			else selectedKeys<<list.at(i)->text(0);
                }
        if (selectedKeys.isEmpty() && !CBsymmetric->isChecked())
                return;

        QStringList returnOptions;
        if (CBuntrusted->isChecked())
                returnOptions<<"--always-trust";
        if (CBarmor->isChecked())
                returnOptions<<"--armor";
        if (CBhideid->isChecked())
                returnOptions<<"--throw-keyid";
        if ((allowcustom) && (!customOptions.stripWhiteSpace().isEmpty()))
                returnOptions.operator+ (QStringList::split(QString(" "),customOptions.simplifyWhiteSpace()));
	//hide();
        if (fmode)
                emit selectedKey(selectedKeys,returnOptions,CBshred->isChecked(),CBsymmetric->isChecked());
        else
                emit selectedKey(selectedKeys,returnOptions,false,false);
        accept();
}

#include "popuppublic.moc"
