/***************************************************************************
                          popuppublic.cpp  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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
#include <qlayout.h>
#include <qpushbutton.h>
#include <qptrlist.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qiconset.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qhbuttongroup.h>
#include <qtoolbutton.h>
#include <qtextcodec.h>

#include <kdeversion.h>
#include <klistview.h>
#include <kprocess.h>
#include <kprocio.h>
#include <klocale.h>
#include <kaccel.h>
#if KDE_IS_VERSION( 3, 2, 90 )
#include <klistviewsearchline.h>
#endif
#include <kactivelabel.h>
#include <kaction.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>


#include "popuppublic.h"
#include "kgpgsettings.h"
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

popupPublic::popupPublic(QWidget *parent, const char *name,QString sfile,bool filemode,KShortcut goDefaultKey):
KDialogBase( Plain, i18n("Select Public Key"), Details | Ok | Cancel, Ok, parent, name,true)
{

	QWidget *page = plainPage();
	QVBoxLayout *vbox=new QVBoxLayout(page,0,spacingHint());
	vbox->setAutoAdd(true);

	setButtonText(KDialogBase::Details,i18n("Options"));

        if (KGpgSettings::allowCustomEncryptionOptions())
                customOptions=KGpgSettings::customEncryptionOptions();

        KIconLoader *loader = KGlobal::iconLoader();

        keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
        keySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
	keyGroup=loader->loadIcon("kgpg_key3",KIcon::Small,20);

        if (filemode) setCaption(i18n("Select Public Key for %1").arg(sfile));
        fmode=filemode;

	QHButtonGroup *hBar=new QHButtonGroup(page);
	//hBar->setFrameStyle(QFrame::NoFrame);
	hBar->setMargin(0);

#if KDE_IS_VERSION( 3, 2, 90 )
	QToolButton *clearSearch = new QToolButton(hBar);
	clearSearch->setTextLabel(i18n("Clear Search"), true);
	clearSearch->setIconSet(SmallIconSet(QApplication::reverseLayout() ? "clear_left"
                                            : "locationbar_erase"));
	(void) new QLabel(i18n("Search: "),hBar);
	KListViewSearchLine* listViewSearch = new KListViewSearchLine(hBar);
	connect(clearSearch, SIGNAL(pressed()), listViewSearch, SLOT(clear()));
#endif

        keysList = new KListView( page );
	 keysList->addColumn(i18n("Name"));
	 keysList->addColumn(i18n("Email"));
	 keysList->addColumn(i18n("ID"));

#if KDE_IS_VERSION( 3, 2, 90 )
	 listViewSearch->setListView(keysList);
#endif

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

	KActionCollection *actcol=new KActionCollection(this);
	(void) new KAction(i18n("&Go to Default Key"),goDefaultKey, this, SLOT(slotGotoDefaultKey()),actcol,"go_default_key");


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
		QWidget *parentBox=new QWidget(boutonboxoptions);
		QHBoxLayout *shredBox=new QHBoxLayout(parentBox,0);
		//shredBox->setFrameStyle(QFrame::NoFrame);
		//shredBox->setMargin(0);
	       CBshred=new QCheckBox(i18n("Shred source file"),parentBox);
                QWhatsThis::add
                        (CBshred,i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));

		QString shredWhatsThis = i18n( "<qt><b>Shred source file:</b><br /><p>Checking this option will shred (overwrite several times before erasing) the files you have encrypted. This way, it is almost impossible that the source file is recovered.</p><p><b>But you must be aware that this is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>");
		  KActiveLabel *warn= new KActiveLabel( i18n("<a href=\"whatsthis:%1\">Read this before using shredding</a>").arg(shredWhatsThis),parentBox );
		  shredBox->addWidget(CBshred);
		  shredBox->addWidget(warn);
        }

	        CBsymmetric=new QCheckBox(i18n("Symmetrical encryption"),boutonboxoptions);
                QWhatsThis::add
                        (CBsymmetric,i18n("<b>Symmetrical encryption</b>: encryption does not use keys. You just need to give a password "
                                          "to encrypt/decrypt the file"));
                QObject::connect(CBsymmetric,SIGNAL(toggled(bool)),this,SLOT(isSymetric(bool)));

	CBarmor->setChecked( KGpgSettings::asciiArmor() );
	CBuntrusted->setChecked( KGpgSettings::allowUntrustedKeys() );
	CBhideid->setChecked( KGpgSettings::hideUserID() );
	if (filemode) CBshred->setChecked( KGpgSettings::shredSource() );

        if (KGpgSettings::allowCustomEncryptionOptions()) {
                QHButtonGroup *bGroup = new QHButtonGroup(page);
                //bGroup->setFrameStyle(QFrame::NoFrame);
                (void) new QLabel(i18n("Custom option:"),bGroup);
                KLineEdit *optiontxt=new KLineEdit(bGroup);
                optiontxt->setText(customOptions);
                QWhatsThis::add
                        (optiontxt,i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));
                QObject::connect(optiontxt,SIGNAL(textChanged ( const QString & )),this,SLOT(customOpts(const QString & )));
        }
        QObject::connect(keysList,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(slotOk()));
//	QObject::connect(this,SIGNAL(okClicked()),this,SLOT(crypte()));
        QObject::connect(CBuntrusted,SIGNAL(toggled(bool)),this,SLOT(refresh(bool)));

        char line[200]="\0";
        FILE *fp2;
        seclist=QString::null;

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys ", "r");
        while ( fgets( line, sizeof(line), fp2))
        {
            QString readLine=QString::fromUtf8(line);
            if (readLine.startsWith("sec")) seclist+=", 0x"+readLine.section(":",4,4).right(8);
        }
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
	keysList->ensureItemVisible(keysList->currentItem());
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

	if (reselect || !keysList->currentItem()->isVisible()) {
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

void popupPublic::slotGotoDefaultKey()
{
    QListViewItem *myDefaulKey = keysList->findItem(KGpgSettings::defaultKey(),2);
    keysList->clearSelection();
    keysList->setCurrentItem(myDefaulKey);
    keysList->setSelected(myDefaulKey,true);
    keysList->ensureItemVisible(myDefaulKey);
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
	QStringList groups= QStringList::split(",", KGpgSettings::groups());
	if (!groups.isEmpty())
	{
		for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
		{
			if (!QString(*it).isEmpty())
			{
				UpdateViewItem2 *item=new UpdateViewItem2(keysList,QString(*it),QString::null,QString::null,false);
				item->setPixmap(0,keyGroup);
			}
		}
	}
        KProcIO *encid=new KProcIO(QTextCodec::codecForLocale());
        *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
        /////////  when process ends, update dialog infos
        QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotpreselect()));
        QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
        encid->start(KProcess::NotifyOnExit,true);
}

void popupPublic::slotpreselect()
{
QListViewItem *it=NULL;
if (!keysList->firstChild()) return;
        if (fmode) it=keysList->findItem(KGpgSettings::defaultKey(),2);
if (!trusted)
              sort();
if (fmode)
{
  	keysList->clearSelection();
	keysList->setSelected(it,true);
	keysList->setCurrentItem(it);
	keysList->ensureItemVisible(it);
}
emit keyListFilled();
}

void popupPublic::slotSetVisible()
{
	keysList->ensureItemVisible(keysList->currentItem());
}

void popupPublic::slotprocread(KProcIO *p)
{
        ///////////////////////////////////////////////////////////////// extract  encryption keys
        bool dead;
        QString tst,keyname,keymail;

        QString defaultKey = KGpgSettings::defaultKey().right(8);

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
                //if (keyname.find("(")!=-1)
                 //       keyname=keyname.section('(',0,0);
        } else {
                keymail=QString::null;
                keyname=tst;//.section('(',0,0);
        }
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


void popupPublic::slotOk()
{
        //////   emit selected data
kdDebug(2100)<<"Ok pressed"<<endl;
        QStringList selectedKeys;
	QString userid;
        QPtrList<QListViewItem> list=keysList->selectedItems();

        for ( uint i = 0; i < list.count(); ++i )
	  if ( list.at(i) && list.at(i)->isVisible()) {
	  		if (!list.at(i)->text(2).isEmpty()) selectedKeys<<list.at(i)->text(2);
			else selectedKeys<<list.at(i)->text(0);
                }
        if (selectedKeys.isEmpty() && !CBsymmetric->isChecked())
                return;
	if (CBsymmetric->isChecked()) selectedKeys=NULL;
kdDebug(2100)<<"Selected Key:"<<selectedKeys<<endl;
        QStringList returnOptions;
        if (CBuntrusted->isChecked())
                returnOptions<<"--always-trust";
        if (CBarmor->isChecked())
                returnOptions<<"--armor";
        if (CBhideid->isChecked())
                returnOptions<<"--throw-keyid";
        if ((KGpgSettings::allowCustomEncryptionOptions()) && (!customOptions.stripWhiteSpace().isEmpty()))
                returnOptions.operator+ (QStringList::split(QString(" "),customOptions.simplifyWhiteSpace()));
	//hide();
        if (fmode)
                emit selectedKey(selectedKeys,returnOptions,CBshred->isChecked(),CBsymmetric->isChecked());
        else
                emit selectedKey(selectedKeys,returnOptions,false,CBsymmetric->isChecked());
        accept();
}

#include "popuppublic.moc"
