/***************************************************************************
                          listkeys.cpp  -  description
                             -------------------
    begin                : Thu Jul 4 2002
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

////////////////////////////////////////////////////// code for the key management

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <qdir.h>
#include <qfile.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qregexp.h>
#include <qpainter.h>
#include <qvbox.h>
#include <qclipboard.h>
#include <qkeysequence.h>
#include <qtextcodec.h>
#include <kstatusbar.h>
#include <qtimer.h>
#include <qpaintdevicemetrics.h>
#include <qtooltip.h>
#include <qheader.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kprocio.h>
#include <qwidget.h>
#include <kaction.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qradiobutton.h>
#include <qpopupmenu.h>

#include <kurlrequester.h>
#include <kio/netaccess.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kshortcut.h>
#include <kstdaccel.h>
#include <klocale.h>
#include <ktip.h>
#include <krun.h>
#include <kprinter.h>
#include <kurldrag.h>
#include <kwin.h>
#include <dcopclient.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kabc/stdaddressbook.h>
#include <kabc/addresseedialog.h>
#include <kdesktopfile.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <qcombobox.h>
#include <qtabwidget.h>
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <kpassivepopup.h>
#include <kfinddialog.h>
#include <kfind.h>
#include <dcopref.h>

#include "newkey.h"
#include "kgpg.h"
#include "kgpgeditor.h"
#include "kgpgview.h"
#include "listkeys.h"
#include "keyexport.h"
#include "sourceselect.h"
#include "adduid.h"
#include "groupedit.h"
#include "kgpgrevokewidget.h"
#include "keyservers.h"
#include "keyserver.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "keygener.h"
#include "kgpgoptions.h"
#include "keyinfowidget.h"

//////////////  KListviewItem special

class UpdateViewItem : public KListViewItem
{
public:
        UpdateViewItem(QListView *parent, QString name,QString email, QString tr, QString val, QString size, QString creat, QString id,bool isdefault,bool isexpired);
	UpdateViewItem(QListViewItem *parent=0, QString name=QString::null,QString email=QString::null, QString tr=QString::null, QString val=QString::null, QString size=QString::null, QString creat=QString::null, QString id=QString::null);
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
        virtual int compare(  QListViewItem * item, int c, bool ascending ) const;
	virtual QString key( int column, bool ) const;
        bool def,exp;
};

UpdateViewItem::UpdateViewItem(QListView *parent, QString name,QString email, QString tr, QString val, QString size, QString creat, QString id,bool isdefault,bool isexpired)
                : KListViewItem(parent)
{
        def=isdefault;
        exp=isexpired;
        setText(0,name);
        setText(1,email);
        setText(2,tr);
        setText(3,val);
        setText(4,size);
        setText(5,creat);
        setText(6,id);
}

UpdateViewItem::UpdateViewItem(QListViewItem *parent, QString name,QString email, QString tr, QString val, QString size, QString creat, QString id)
                : KListViewItem(parent)
{
        setText(0,name);
        setText(1,email);
        setText(2,tr);
        setText(3,val);
        setText(4,size);
        setText(5,creat);
        setText(6,id);
}


void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        QColorGroup _cg( cg );
	if (depth()==0)
	{
	if ((def) && (column<2)) {
                QFont font(p->font());
                font.setBold(true);
                p->setFont(font);
        }
	else if ((exp) && (column==3)) _cg.setColor( QColorGroup::Text, Qt::red );
	}
	else
        if (column<2) {
                QFont font(p->font());
                font.setItalic(true);
                p->setFont(font);
        }


        KListViewItem::paintCell(p,_cg, column, width, alignment);
}

#include <iostream>
using namespace std;

int UpdateViewItem :: compare(  QListViewItem * item, int c, bool ascending ) const
{
        int rc = 0;
        if ((c==3) || (c==5)) {
                QDate d = KGlobal::locale()->readDate(text(c));
                QDate itemDate = KGlobal::locale()->readDate(item->text(c));
                bool itemDateValid = itemDate.isValid();
                if (d.isValid()) {
                        if (itemDateValid) {
                                if (d < itemDate)
                                        rc = -1;
                                else if (d > itemDate)
                                        rc = 1;
                        } else
                                rc = -1;
                } else if (itemDateValid)
                        rc = 1;
        return rc;
	}
	if (c==2)   /* sorting by pixmap */
        {
                const QPixmap* pix = pixmap(c);
                const QPixmap* itemPix = item->pixmap(c);
                int serial,itemSerial;
                if (!pix)
                        serial=0;
                else
                        serial=pix->serialNumber();
                if (!itemPix)
                        itemSerial=0;
                else
                        itemSerial=itemPix->serialNumber();
                if (serial<itemSerial)
                        rc=-1;
                else if (serial>itemSerial)
                        rc=1;
		return rc;
        }
	return QListViewItem::compare(item,c,ascending);
}

QString UpdateViewItem::key( int column, bool ) const
{
    return text( column ).lower();
}


////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////   Secret key selection dialog, used when user wants to sign a key
KgpgSelKey::KgpgSelKey(QWidget *parent, const char *name,bool allowMultipleSelection, QString preselected):
KDialogBase( parent, name, true,i18n("Private Key List"),Ok | Cancel)
{
        QString keyname;
        page = new QWidget(this);
        QLabel *labeltxt;
        KIconLoader *loader = KGlobal::iconLoader();
        keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);

        setMinimumSize(350,100);
        keysListpr = new KListView( page );
        keysListpr->setRootIsDecorated(true);
        keysListpr->addColumn( i18n( "Name" ));
	keysListpr->addColumn( i18n( "Email" ));
	keysListpr->addColumn( i18n( "ID" ));
        keysListpr->setShowSortIndicator(true);
        keysListpr->setFullWidth(true);
	keysListpr->setAllColumnsShowFocus(true);
	if (allowMultipleSelection) keysListpr->setSelectionMode(QListView::Extended);

        labeltxt=new QLabel(i18n("Choose secret key:"),page);
        vbox=new QVBoxLayout(page);

        if (preselected==QString::null) preselected = KGpgSettings::defaultKey();

        FILE *fp,*fp2;
        QString fullname,tst,tst2;
        char line[300];

        bool selectedok=false;
	bool warn=false;
	KListViewItem *item;

        fp = popen("gpg --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=QString::fromUtf8(line);
                if (tst.startsWith("sec")) {
                        QStringList keyString=QStringList::split(":",tst,true);
                        QString val=keyString[6];
                        QString id=QString("0x"+keyString[4].right(8));
                        if (val.isEmpty())
                                val=i18n("Unlimited");
                        fullname=keyString[9];

                        fp2 = popen(QFile::encodeName(QString("gpg --no-tty --with-colons --list-key %1").arg(KShellProcess::quote(id))), "r");
                        bool dead=true;
                        while ( fgets( line, sizeof(line), fp2)) {
                                tst2=QString::fromUtf8(line);
                                if (tst2.startsWith("pub")) {
                         const QString trust2=tst2.section(':',1,1);
                        switch( trust2[0] ) {
                        case 'f':
				dead=false;
                                break;
                        case 'u':
				dead=false;
                                break;
			case '-':
				if (tst2.section(':',11,11).find('D')==-1) warn=true;
				break;
                        default:
                                break;
                        }
                                        if (tst2.section(':',11,11).find('D')!=-1)
                                                dead=true;
					break;
                                }
                        }
                        pclose(fp2);
                        if (!fullname.isEmpty() && (!dead)) {
			QString keyMail,keyName;
			if (fullname.find("<")!=-1) {
                	keyMail=fullname.section('<',-1,-1);
                	keyMail.truncate(keyMail.length()-1);
                	keyName=fullname.section('<',0,0);
			} else {
                	keyMail=QString::null;
			keyName=fullname;
        		}

        		keyName=KgpgInterface::checkForUtf8(keyName);


                                item=new KListViewItem(keysListpr,keyName,keyMail,id);
                                //KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, expiration: %3").arg(id).arg(trust).arg(val));
				KListViewItem *sub= new KListViewItem(item,i18n("Expiration:"),val);
                                sub->setSelectable(false);
                                item->setPixmap(0,keyPair);
                                if (preselected.find(id,0,false)!=-1) {
                                        keysListpr->setSelected(item,true);
					keysListpr->setCurrentItem(item);
                                        selectedok=true;
                                }
                        }
                }
        }
        pclose(fp);

	if (warn)
	{
	KMessageBox::information(this,i18n("<qt><b>Some of your secret keys are untrusted.</b><br>Change their trust if you want to use them for signing.</qt>"),QString::null,"warnUntrusted");
	}
        QObject::connect(keysListpr,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(slotpreOk()));
        QObject::connect(keysListpr,SIGNAL(clicked(QListViewItem *)),this,SLOT(slotSelect(QListViewItem *)));


        if (!selectedok)
	{
                keysListpr->setSelected(keysListpr->firstChild(),true);
		keysListpr->setCurrentItem(keysListpr->firstChild());
	}

        vbox->addWidget(labeltxt);
        vbox->addWidget(keysListpr);
        setMainWidget(page);
}


void KgpgSelKey::slotpreOk()
{
        if (keysListpr->currentItem()->depth()!=0)
                return;
        else
                slotOk();
}

void KgpgSelKey::slotOk()
{
        if (keysListpr->currentItem()==NULL)
                reject();
        else
                accept();
}

void KgpgSelKey::slotSelect(QListViewItem *item)
{
        if (item==NULL)
                return;
        if (item->depth()!=0) {
                keysListpr->setSelected(item->parent(),true);
                keysListpr->setCurrentItem(item->parent());
        }
}


QString KgpgSelKey::getkeyID()
{
        /////  emit selected key
        if (keysListpr->currentItem()==NULL)
                return(QString::null);
	QString result;
	QPtrList< QListViewItem > list = keysListpr->selectedItems(false);
	QListViewItem *item;
    	for ( item = list.first(); item; item = list.next() )
	{
	result.append(item->text(2));
	if (item!=list.getLast()) result.append(" ");
	}
        return(result);
}

QString KgpgSelKey::getkeyMail()
{
        QString username;
        /////  emit selected key
        if (keysListpr->currentItem()==NULL)
                return(QString::null);
        else {
                username=keysListpr->currentItem()->text(0);
                //username=username.section(' ',0,0);
                username=username.stripWhiteSpace();
                return(username);
        }
}



/////////////////////////////////////////////////////////////////////////////////////////////

KeyView::KeyView( QWidget *parent, const char *name )
                : KListView( parent, name )
{
        KIconLoader *loader = KGlobal::iconLoader();

        pixkeyOrphan=loader->loadIcon("kgpg_key4",KIcon::Small,20);
        pixkeyGroup=loader->loadIcon("kgpg_key3",KIcon::Small,20);
        pixkeyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
        pixkeySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
        pixsignature=loader->loadIcon("signature",KIcon::Small,20);
        pixuserid=loader->loadIcon("kgpg_identity",KIcon::Small,20);
        pixuserphoto=loader->loadIcon("kgpg_photo",KIcon::Small,20);
        pixRevoke=loader->loadIcon("stop",KIcon::Small,20);
	QPixmap blankFrame;
	blankFrame.load(locate("appdata", "pics/kgpg_blank.png"));

	trustunknown.load(locate("appdata", "pics/kgpg_fill.png"));
	trustunknown.fill(KGpgSettings::colorUnknown());
	bitBlt(&trustunknown,0,0,&blankFrame,0,0,50,15);

	trustbad.load(locate("appdata", "pics/kgpg_fill.png"));
	trustbad.fill(KGpgSettings::colorBad());//QColor(172,0,0));
	bitBlt(&trustbad,0,0,&blankFrame,0,0,50,15);

	trustrevoked.load(locate("appdata", "pics/kgpg_fill.png"));
	trustrevoked.fill(KGpgSettings::colorRev());//QColor(30,30,30));
	bitBlt(&trustrevoked,0,0,&blankFrame,0,0,50,15);

	trustgood.load(locate("appdata", "pics/kgpg_fill.png"));
	trustgood.fill(KGpgSettings::colorGood());//QColor(144,255,0));
	bitBlt(&trustgood,0,0,&blankFrame,0,0,50,15);

        connect(this,SIGNAL(expanded (QListViewItem *)),this,SLOT(expandKey(QListViewItem *)));
        header()->setMovingEnabled(false);
        setAcceptDrops(true);
        setDragEnabled(true);
}



void  KeyView::droppedfile (KURL url)
{
        if (KMessageBox::questionYesNo(this,i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>").arg(url.path()), QString::null, i18n("Import"), i18n("Do Not Import"))!=KMessageBox::Yes)
                return;

        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKeyURL(url);
        connect(importKeyProcess,SIGNAL(importfinished(QStringList)),this,SLOT(slotReloadKeys(QStringList)));
}

void KeyView::contentsDragMoveEvent(QDragMoveEvent *e)
{
        e->accept (KURLDrag::canDecode(e));
}

void  KeyView::contentsDropEvent (QDropEvent *o)
{
        KURL::List list;
        if ( KURLDrag::decode( o, list ) )
                droppedfile(list.first());
}

void  KeyView::startDrag()
{
        FILE *fp;
        char line[200]="";
        QString keyid=currentItem()->text(6);
        if (!keyid.startsWith("0x"))
                return;
        QString gpgcmd="gpg --display-charset=utf-8 --no-tty --export --armor "+KShellProcess::quote(keyid.local8Bit());

        QString keytxt;
        fp=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), fp))    /// read output
                if (!QString(line).startsWith("gpg:"))
                        keytxt+=QString::fromUtf8(line);
        pclose(fp);

        QDragObject *d = new QTextDrag( keytxt, this );
        d->dragCopy();
        // do NOT delete d.
}


mySearchLine::mySearchLine(QWidget *parent, KeyView *listView, const char *name)
:KListViewSearchLine(parent,listView,name)
{
searchListView=listView;
setKeepParentsVisible(false);
}

mySearchLine::~ mySearchLine()
{}


bool mySearchLine::itemMatches(const QListViewItem *item, const QString & s) const
{
if (item->depth()!=0) return true;
else return KListViewSearchLine::itemMatches(item,s);
}



void mySearchLine::updateSearch(const QString& s)
{
    KListViewSearchLine::updateSearch(s);
    if (searchListView->displayOnlySecret || !searchListView->displayDisabled)
    {
    int disabledSerial=searchListView->trustbad.serialNumber();
	QListViewItem *item=searchListView->firstChild();
	while (item)
	{
	    if (item->isVisible() && !(item->text(6).isEmpty()))
	    {
		if (searchListView->displayOnlySecret && searchListView->secretList.find(item->text(6))==-1)
                    item->setVisible(false);
		if (!searchListView->displayDisabled && item->pixmap(2))
		    if (item->pixmap(2)->serialNumber()==disabledSerial)
                        item->setVisible(false);
            }
	 item=item->nextSibling();
	 }
    }
}

///////////////////////////////////////////////////////////////////////////////////////   main window for key management

listKeys::listKeys(QWidget *parent, const char *name) : DCOPObject( "KeyInterface" ), KMainWindow(parent, name,0)
{
        //KWin::setType(Qt::WDestructiveClose);

        keysList2 = new KeyView(this);
        keysList2->photoKeysList=QString::null;
        keysList2->groupNb=0;
	keyStatusBar=NULL;
        readOptions();

        if (showTipOfDay)
                installEventFilter(this);
        setCaption(i18n("Key Management"));

        (void) new KAction(i18n("&Open Editor"), "edit",0,this, SLOT(slotOpenEditor()),actionCollection(),"kgpg_editor");
        KAction *exportPublicKey = new KAction(i18n("E&xport Public Keys..."), "kgpg_export", KStdAccel::shortcut(KStdAccel::Copy),this, SLOT(slotexport()),actionCollection(),"key_export");
        KAction *deleteKey = new KAction(i18n("&Delete Keys"),"editdelete", Qt::Key_Delete,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
        signKey = new KAction(i18n("&Sign Keys..."), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
        KAction *delSignKey = new KAction(i18n("Delete Sign&ature"),"editdelete", 0,this, SLOT(delsignkey()),actionCollection(),"key_delsign");
        KAction *infoKey = new KAction(i18n("&Edit Key"), "kgpg_info", Qt::Key_Return,this, SLOT(listsigns()),actionCollection(),"key_info");
        KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste),this, SLOT(slotPreImportKey()),actionCollection(),"key_import");
        KAction *setDefaultKey = new KAction(i18n("Set as De&fault Key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
        importSignatureKey = new KAction(i18n("Import Key From Keyserver"),"network", 0,this, SLOT(preimportsignkey()),actionCollection(),"key_importsign");
        importAllSignKeys = new KAction(i18n("Import &Missing Signatures From Keyserver"),"network", 0,this, SLOT(importallsignkey()),actionCollection(),"key_importallsign");
        refreshKey = new KAction(i18n("&Refresh Keys From Keyserver"),"reload", 0,this, SLOT(refreshKeyFromServer()),actionCollection(),"key_server_refresh");

	KAction *createGroup=new KAction(i18n("&Create Group with Selected Keys..."), 0, 0,this, SLOT(createNewGroup()),actionCollection(),"create_group");
        KAction *delGroup= new KAction(i18n("&Delete Group"), 0, 0,this, SLOT(deleteGroup()),actionCollection(),"delete_group");
        KAction *editCurrentGroup= new KAction(i18n("&Edit Group"), 0, 0,this, SLOT(editGroup()),actionCollection(),"edit_group");

	KAction *newContact=new KAction(i18n("&Create New Contact in Address Book"), "kaddressbook", 0,this, SLOT(addToKAB()),actionCollection(),"add_kab");
        (void) new KAction(i18n("&Go to Default Key"), "gohome",QKeySequence(CTRL+Qt::Key_Home) ,this, SLOT(slotGotoDefaultKey()),actionCollection(),"go_default_key");

        KStdAction::quit(this, SLOT(quitApp()), actionCollection());
        KStdAction::find(this, SLOT(findKey()), actionCollection());
        KStdAction::findNext(this, SLOT(findNextKey()), actionCollection());
        (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(),this, SLOT(refreshkey()),actionCollection(),"key_refresh");
        KAction *openPhoto= new KAction(i18n("&Open Photo"), "image", 0,this, SLOT(slotShowPhoto()),actionCollection(),"key_photo");
        KAction *deletePhoto= new KAction(i18n("&Delete Photo"), "delete", 0,this, SLOT(slotDeletePhoto()),actionCollection(),"delete_photo");
        KAction *addPhoto= new KAction(i18n("&Add Photo"), 0, 0,this, SLOT(slotAddPhoto()),actionCollection(),"add_photo");

        KAction *addUid= new KAction(i18n("&Add User Id"), 0, 0,this, SLOT(slotAddUid()),actionCollection(),"add_uid");
        KAction *delUid= new KAction(i18n("&Delete User Id"), 0, 0,this, SLOT(slotDelUid()),actionCollection(),"del_uid");

        KAction *editKey = new KAction(i18n("Edit Key in &Terminal"), "kgpg_term", QKeySequence(ALT+Qt::Key_Return),this, SLOT(slotedit()),actionCollection(),"key_edit");
        KAction *exportSecretKey = new KAction(i18n("Export Secret Key..."), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
        KAction *revokeKey = new KAction(i18n("Revoke Key..."), 0, 0,this, SLOT(revokeWidget()),actionCollection(),"key_revoke");

        KAction *deleteKeyPair = new KAction(i18n("Delete Key Pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
        KAction *generateKey = new KAction(i18n("&Generate Key Pair..."), "kgpg_gen", KStdAccel::shortcut(KStdAccel::New),this, SLOT(slotgenkey()),actionCollection(),"key_gener");

        KAction *regeneratePublic = new KAction(i18n("&Regenerate Public Key"), 0, 0,this, SLOT(slotregenerate()),actionCollection(),"key_regener");

        (void) new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(showKeyServer()),actionCollection(),"key_server");
        KStdAction::preferences(this, SLOT(showOptions()), actionCollection(),"options_configure");
        (void) new KAction(i18n("Tip of the &Day"), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
        (void) new KAction(i18n("View GnuPG Manual"), "contents", 0,this, SLOT(slotManpage()),actionCollection(),"gpg_man");

        (void) new KToggleAction(i18n("&Show only Secret Keys"), "kgpg_show", 0,this, SLOT(slotToggleSecret()),actionCollection(),"show_secret");
        keysList2->displayOnlySecret=false;

	(void) new KToggleAction(i18n("&Hide Expired/Disabled Keys"),0, 0,this, SLOT(slotToggleDisabled()),actionCollection(),"hide_disabled");
	keysList2->displayDisabled=true;

        sTrust=new KToggleAction(i18n("Trust"),0, 0,this, SLOT(slotShowTrust()),actionCollection(),"show_trust");
        sSize=new KToggleAction(i18n("Size"),0, 0,this, SLOT(slotShowSize()),actionCollection(),"show_size");
        sCreat=new KToggleAction(i18n("Creation"),0, 0,this, SLOT(slotShowCreat()),actionCollection(),"show_creat");
        sExpi=new KToggleAction(i18n("Expiration"),0, 0,this, SLOT(slotShowExpi()),actionCollection(),"show_expi");


        photoProps = new KSelectAction(i18n("&Photo ID's"),"kgpg_photo", actionCollection(), "photo_settings");
        connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));

        // Keep the list in kgpg.kcfg in sync with this one!
        QStringList list;
        list.append(i18n("Disable"));
        list.append(i18n("Small"));
        list.append(i18n("Medium"));
        list.append(i18n("Large"));
        photoProps->setItems(list);

        int pSize = KGpgSettings::photoProperties();
        photoProps->setCurrentItem( pSize );
        slotSetPhotoSize(pSize);

        keysList2->setRootIsDecorated(true);
        keysList2->addColumn( i18n( "Name" ),200);
        keysList2->addColumn( i18n( "Email" ),200);
        keysList2->addColumn( i18n( "Trust" ),60);
        keysList2->addColumn( i18n( "Expiration" ),100);
        keysList2->addColumn( i18n( "Size" ),100);
        keysList2->addColumn( i18n( "Creation" ),100);
        keysList2->addColumn( i18n( "Id" ),100);
        keysList2->setShowSortIndicator(true);
        keysList2->setAllColumnsShowFocus(true);
        keysList2->setFullWidth(true);
        keysList2->setAcceptDrops (true) ;
        keysList2->setSelectionModeExt(KListView::Extended);


        popup=new QPopupMenu();
        exportPublicKey->plug(popup);
        deleteKey->plug(popup);
        signKey->plug(popup);
        infoKey->plug(popup);
        editKey->plug(popup);
        refreshKey->plug(popup);
        setDefaultKey->plug(popup);
        popup->insertSeparator();
        importAllSignKeys->plug(popup);

        popupsec=new QPopupMenu();
        exportPublicKey->plug(popupsec);
        signKey->plug(popupsec);
        infoKey->plug(popupsec);
        editKey->plug(popupsec);
        refreshKey->plug(popupsec);
        setDefaultKey->plug(popupsec);
        popupsec->insertSeparator();
        importAllSignKeys->plug(popupsec);
        popupsec->insertSeparator();
        addPhoto->plug(popupsec);
        addUid->plug(popupsec);
        exportSecretKey->plug(popupsec);
        deleteKeyPair->plug(popupsec);
        revokeKey->plug(popupsec);

        popupgroup=new QPopupMenu();
        editCurrentGroup->plug(popupgroup);
        delGroup->plug(popupgroup);

        popupout=new QPopupMenu();
        importKey->plug(popupout);
        generateKey->plug(popupout);

        popupsig=new QPopupMenu();
        importSignatureKey->plug(popupsig);
        delSignKey->plug(popupsig);

        popupphoto=new QPopupMenu();
        openPhoto->plug(popupphoto);
        deletePhoto->plug(popupphoto);

        popupuid=new QPopupMenu();
        delUid->plug(popupuid);

        popuporphan=new QPopupMenu();
        regeneratePublic->plug(popuporphan);
        deleteKeyPair->plug(popuporphan);

	editCurrentGroup->setEnabled(false);
	delGroup->setEnabled(false);
	createGroup->setEnabled(false);
	infoKey->setEnabled(false);
	editKey->setEnabled(false);
	signKey->setEnabled(false);
	refreshKey->setEnabled(false);
	exportPublicKey->setEnabled(false);
	newContact->setEnabled(false);

        setCentralWidget(keysList2);
        keysList2->restoreLayout(KGlobal::config(), "KeyView");

        QObject::connect(keysList2,SIGNAL(returnPressed(QListViewItem *)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(selectionChanged ()),this,SLOT(checkList()));
        QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                         this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));
        QObject::connect(keysList2,SIGNAL(destroyed()),this,SLOT(annule()));


        ///////////////    get all keys data
	keyStatusBar=statusBar();

	setupGUI(KMainWindow::Create | Save | ToolBar | StatusBar | Keys, "listkeys.rc");
        toolBar()->insertLineSeparator();

	QToolButton *clearSearch = new QToolButton(toolBar());
	clearSearch->setTextLabel(i18n("Clear Search"), true);
	clearSearch->setIconSet(SmallIconSet(QApplication::reverseLayout() ? "clear_left"
                                            : "locationbar_erase"));
	(void) new QLabel(i18n("Search: "),toolBar());
	listViewSearch = new mySearchLine(toolBar(),keysList2);
	connect(clearSearch, SIGNAL(pressed()), listViewSearch, SLOT(clear()));


	(void)new KAction(i18n("Filter Search"), Qt::Key_F6, listViewSearch, SLOT(setFocus()),actionCollection(), "search_focus");

        sTrust->setChecked(KGpgSettings::showTrust());
        sSize->setChecked(KGpgSettings::showSize());
        sCreat->setChecked(KGpgSettings::showCreat());
        sExpi->setChecked(KGpgSettings::showExpi());

        statusbarTimer = new QTimer(this);

        keyStatusBar->insertItem("",0,1);
        keyStatusBar->insertFixedItem(i18n("00000 Keys, 000 Groups"),1,true);
        keyStatusBar->setItemAlignment(0, AlignLeft);
        keyStatusBar->changeItem("",1);
        QObject::connect(keysList2,SIGNAL(statusMessage(QString,int,bool)),this,SLOT(changeMessage(QString,int,bool)));
        QObject::connect(statusbarTimer,SIGNAL(timeout()),this,SLOT(statusBarTimeout()));

	s_kgpgEditor= new KgpgApp(parent, "editor",WType_Dialog,actionCollection()->action("go_default_key")->shortcut(),true);
        connect(s_kgpgEditor,SIGNAL(refreshImported(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
        connect(this,SIGNAL(fontChanged(QFont)),s_kgpgEditor,SLOT(slotSetFont(QFont)));
        connect(s_kgpgEditor->view->editor,SIGNAL(refreshImported(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
}


listKeys::~listKeys()
{}

void  listKeys::showKeyManager()
{
show();
}

void  listKeys::slotOpenEditor()
{
  KgpgApp *kgpgtxtedit = new KgpgApp(this, "editor",WType_TopLevel | WDestructiveClose,actionCollection()->action("go_default_key")->shortcut());
        connect(kgpgtxtedit,SIGNAL(refreshImported(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
	connect(kgpgtxtedit,SIGNAL(encryptFiles(KURL::List)),this,SIGNAL(encryptFiles(KURL::List)));
        connect(this,SIGNAL(fontChanged(QFont)),kgpgtxtedit,SLOT(slotSetFont(QFont)));
        connect(kgpgtxtedit->view->editor,SIGNAL(refreshImported(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
        kgpgtxtedit->show();
}

void listKeys::statusBarTimeout()
{
        keyStatusBar->changeItem("",0);
}

void listKeys::changeMessage(QString msg, int nb, bool keep)
{
        statusbarTimer->stop();
        if ((nb==0) & (!keep))
                statusbarTimer->start(10000, true);
        keyStatusBar->changeItem(" "+msg+" ",nb);
}


void KeyView::slotRemoveColumn(int d)
{
        hideColumn(d);
        header()->setResizeEnabled(false,d);
        header()->setStretchEnabled(true,6);
}

void KeyView::slotAddColumn(int c)
{
        header()->setResizeEnabled(true,c);
        adjustColumn(c);
}

void listKeys::slotShowTrust()
{
        if (sTrust->isChecked())
                keysList2->slotAddColumn(2);
        else
                keysList2->slotRemoveColumn(2);
}

void listKeys::slotShowExpi()
{
        if (sExpi->isChecked())
                keysList2->slotAddColumn(3);
        else
                keysList2->slotRemoveColumn(3);
}

void listKeys::slotShowSize()
{
        if (sSize->isChecked())
                keysList2->slotAddColumn(4);
        else
                keysList2->slotRemoveColumn(4);
}

void listKeys::slotShowCreat()
{
        if (sCreat->isChecked())
                keysList2->slotAddColumn(5);
        else
                keysList2->slotRemoveColumn(5);
}


bool listKeys::eventFilter( QObject *, QEvent *e )
{
        if ((e->type() == QEvent::Show) && (showTipOfDay)) {
                KTipDialog::showTip(this, QString("kgpg/tips"), false);
                showTipOfDay=false;
        }
        return FALSE;
}


void listKeys::slotToggleSecret()
{
        QListViewItem *item=keysList2->firstChild();
        if (!item)
                return;

        keysList2->displayOnlySecret=!keysList2->displayOnlySecret;
	listViewSearch->updateSearch(listViewSearch->text());
}

void listKeys::slotToggleDisabled()
{
       QListViewItem *item=keysList2->firstChild();
        if (!item)
                return;

        keysList2->displayDisabled=!keysList2->displayDisabled;
	listViewSearch->updateSearch(listViewSearch->text());
}

void listKeys::slotGotoDefaultKey()
{
        QListViewItem *myDefaulKey = keysList2->findItem(KGpgSettings::defaultKey(),6);
        keysList2->clearSelection();
        keysList2->setCurrentItem(myDefaulKey);
        keysList2->setSelected(myDefaulKey,true);
        keysList2->ensureItemVisible(myDefaulKey);
}



void listKeys::refreshKeyFromServer()
{
        if (keysList2->currentItem()==NULL)
                return;
        QString keyIDS;
        keysList=keysList2->selectedItems();
        bool keyDepth=true;
        for ( uint i = 0; i < keysList.count(); ++i )
                if ( keysList.at(i) ) {
                        if ((keysList.at(i)->depth()!=0) || (keysList.at(i)->text(6).isEmpty()))
                                keyDepth=false;
                        else
                                keyIDS+=keysList.at(i)->text(6)+" ";
                }
        if (!keyDepth) {
                KMessageBox::sorry(this,i18n("You can only refresh primary keys. Please check your selection."));
                return;
        }
        kServer=new keyServer(0,"server_dialog",false);
        kServer->page->kLEimportid->setText(keyIDS);
        kServer->slotImport();
        connect( kServer, SIGNAL( importFinished(QString) ) , this, SLOT(refreshFinished()));
}


void listKeys::refreshFinished()
{
        if (kServer)
                kServer=0L;

        for ( uint i = 0; i < keysList.count(); ++i )
                if (keysList.at(i))
                        keysList2->refreshcurrentkey(keysList.at(i));
}


void listKeys::slotDelUid()
{
        QListViewItem *item=keysList2->currentItem();
        while (item->depth()>0)
                item=item->parent();

        KProcess *conprocess=new KProcess();
        KConfig *config = KGlobal::config();
        config->setGroup("General");
        *conprocess<< config->readPathEntry("TerminalApplication","konsole");
        *conprocess<<"-e"<<"gpg";
        *conprocess<<"--edit-key"<<item->text(6)<<"uid";
        conprocess->start(KProcess::Block);
        keysList2->refreshselfkey();
}


void listKeys::slotregenerate()
{
        FILE *fp;
        QString tst;
        char line[300];
        QString cmd="gpg --display-charset=utf-8 --no-secmem-warning --export-secret-key "+keysList2->currentItem()->text(6)+" | gpgsplit --no-split --secret-to-public | gpg --import";

        fp = popen(QFile::encodeName(cmd), "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst+=QString::fromUtf8(line);
        }
        pclose(fp);
        QString regID=keysList2->currentItem()->text(6);
        keysList2->takeItem(keysList2->currentItem());
        keysList2->refreshcurrentkey(regID);
}

void listKeys::slotAddUid()
{
        addUidWidget=new KDialogBase(KDialogBase::Swallow, i18n("Add New User Id"),  KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,this,0,true);
        addUidWidget->enableButtonOK(false);
        AddUid *keyUid=new AddUid();
        addUidWidget->setMainWidget(keyUid);
        //keyUid->setMinimumSize(keyUid->sizeHint());
        keyUid->setMinimumWidth(300);
        connect(keyUid->kLineEdit1,SIGNAL(textChanged ( const QString & )),this,SLOT(slotAddUidEnable(const QString & )));
        if (addUidWidget->exec()!=QDialog::Accepted)
                return;
        KgpgInterface *addUidProcess=new KgpgInterface();
        addUidProcess->KgpgAddUid(keysList2->currentItem()->text(6),keyUid->kLineEdit1->text(),keyUid->kLineEdit2->text(),keyUid->kLineEdit3->text());
        connect(addUidProcess,SIGNAL(addUidFinished()),keysList2,SLOT(refreshselfkey()));
        connect(addUidProcess,SIGNAL(addUidError(QString)),this,SLOT(slotGpgError(QString)));
}

void listKeys::slotAddUidEnable(const QString & name)
{
        addUidWidget->enableButtonOK(name.length()>4);
}


void listKeys::slotAddPhoto()
{
        QString mess=i18n("The image must be a JPEG file. Remember that the image is stored within your public key."
                          "If you use a very large picture, your key will become very large as well! Keeping the image "
                          "close to 240x288 is a good size to use.");

        if (KMessageBox::warningContinueCancel(this,mess)!=KMessageBox::Continue)
                return;

        QString imagePath=KFileDialog::getOpenFileName (QString::null,"image/jpeg",this);
        if (imagePath.isEmpty())
                return;
        KgpgInterface *addPhotoProcess=new KgpgInterface();
        addPhotoProcess->KgpgAddPhoto(keysList2->currentItem()->text(6),imagePath);
        connect(addPhotoProcess,SIGNAL(addPhotoFinished()),this,SLOT(slotUpdatePhoto()));
        connect(addPhotoProcess,SIGNAL(addPhotoError(QString)),this,SLOT(slotGpgError(QString)));
}

void listKeys::slotGpgError(QString errortxt)
{
        KMessageBox::detailedSorry(this,i18n("Something unexpected happened during the requested operation.\nPlease check details for full log output."),errortxt);
}


void listKeys::slotDeletePhoto()
{
        if (KMessageBox::warningContinueCancel(this,i18n("<qt>Are you sure you want to delete Photo id <b>%1</b><br>from key <b>%2 &lt;%3&gt;</b> ?</qt>").arg(keysList2->currentItem()->text(6)).arg(keysList2->currentItem()->parent()->text(0)).arg(keysList2->currentItem()->parent()->text(1)),i18n("Warning"),KGuiItem(i18n("Delete"),"editdelete"))!=KMessageBox::Continue)
                return;

        KgpgInterface *delPhotoProcess=new KgpgInterface();
        delPhotoProcess->KgpgDeletePhoto(keysList2->currentItem()->parent()->text(6),keysList2->currentItem()->text(6));
        connect(delPhotoProcess,SIGNAL(delPhotoFinished()),this,SLOT(slotUpdatePhoto()));
        connect(delPhotoProcess,SIGNAL(delPhotoError(QString)),this,SLOT(slotGpgError(QString)));
}

void listKeys::slotUpdatePhoto()
{
        keysList2->refreshselfkey();
}


void listKeys::slotSetPhotoSize(int size)
{
        switch( size) {
        case 1:
                showPhoto=true;
                keysList2->previewSize=22;
                break;
        case 2:
                showPhoto=true;
                keysList2->previewSize=42;
                break;
        case 3:
                showPhoto=true;
                keysList2->previewSize=65;
                break;
        default:
                showPhoto=false;
                break;
        }
        keysList2->displayPhoto=showPhoto;

        /////////////////////////////   refresh keys with photo id

        QListViewItem *newdef = keysList2->firstChild();
        while (newdef) {
                //if ((keysList2->photoKeysList.find(newdef->text(6))!=-1) && (newdef->childCount ()>0))
                if (newdef->childCount ()>0) {
                        bool hasphoto=false;
                        QListViewItem *newdefChild = newdef->firstChild();
                        while (newdefChild) {
                                if (newdefChild->text(0)==i18n("Photo id")) {
                                        hasphoto=true;
                                        break;
                                }
                                newdefChild = newdefChild->nextSibling();
                        }
                        if (hasphoto) {
                                while (newdef->firstChild())
                                        delete newdef->firstChild();
                                keysList2->expandKey(newdef);
                        }
                }
                newdef = newdef->nextSibling();
        }
}

void listKeys::findKey()
{
        KFindDialog fd(this,"find_dialog",0,"");
        if ( fd.exec() != QDialog::Accepted )
                return;
        searchString=fd.pattern();
        searchOptions=fd.options();
        findFirstKey();
}

void listKeys::findFirstKey()
{
        if (searchString.isEmpty())
                return;
        bool foundItem=true;
        QListViewItem *item=keysList2->firstChild();
        if (!item)
                return;
        QString searchText=item->text(0)+" "+item->text(1)+" "+item->text(6);


        //
        KFind *m_find = new KFind(searchString, searchOptions,this);
        m_find->setData(searchText);
        while (m_find->find()==KFind::NoMatch) {
                if (!item->nextSibling()) {
                        foundItem=false;
                        break;
                } else {
                        item = item->nextSibling();
                        searchText=item->text(0)+" "+item->text(1)+" "+item->text(6);
                        m_find->setData(searchText);
                }
        }
        delete m_find;

        if (foundItem) {

                keysList2->clearSelection();
                keysList2->setCurrentItem(item);
                keysList2->setSelected(item,true);
                keysList2->ensureItemVisible(item);
        } else
                KMessageBox::sorry(this,i18n("<qt>Search string '<b>%1</b>' not found.").arg(searchString));
}

void listKeys::findNextKey()
{
				//kdDebug(2100)<<"find next"<<endl;
        if (searchString.isEmpty()) {
                findKey();
                return;
        }
        bool foundItem=true;
        QListViewItem *item=keysList2->currentItem();
        if (!item)
                return;
        while(item->depth() > 0)
                item = item->parent();
        item=item->nextSibling();
        QString searchText=item->text(0)+" "+item->text(1)+" "+item->text(6);
        //kdDebug(2100)<<"Next string:"<<searchText<<endl;
        //kdDebug(2100)<<"Search:"<<searchString<<endl;
        //kdDebug(2100)<<"OPts:"<<searchOptions<<endl;
        KFind *m_find = new KFind(searchString, searchOptions,this);
        m_find->setData(searchText);
        while (m_find->find()==KFind::NoMatch) {
                if (!item->nextSibling()) {
                        foundItem=false;
                        break;
                } else {
                        item = item->nextSibling();
                        searchText=item->text(0)+" "+item->text(1)+" "+item->text(6);
                        m_find->setData(searchText);
                        //kdDebug(2100)<<"Next string:"<<searchText<<endl;
                }
        }
        delete m_find;
        if (foundItem) {
                keysList2->clearSelection();
                keysList2->setCurrentItem(item);
                keysList2->setSelected(item,true);
                keysList2->ensureItemVisible(item);
        } else
                findFirstKey();
}




void listKeys::addToKAB()
{
        KABC::Key key;
	if (!keysList2->currentItem()) return;
        //QString email=extractKeyMail(keysList2->currentItem()).stripWhiteSpace();
        QString email=keysList2->currentItem()->text(1);

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the address book. Please check your installation."));
                return;
        }

	KABC::Addressee::List addresseeList = ab->findByEmail(email);
  	kapp->startServiceByDesktopName( "kaddressbook" );
  	DCOPRef call( "kaddressbook", "KAddressBookIface" );
  	if( !addresseeList.isEmpty() ) {
    	call.send( "showContactEditor(QString)", addresseeList.first().uid() );
  	}
  	else {
    	call.send( "addEmail(QString)", QString (keysList2->currentItem()->text(0))+" <"+email+">" );
  	}
}

/*
void listKeys::allToKAB()
{
        KABC::Key key;
        QString email;
        QStringList keylist;
        KABC::Addressee a;

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the address book. Please check your installation."));
                return;
        }

        QListViewItem * myChild = keysList2->firstChild();
        while( myChild ) {
                //email=extractKeyMail(myChild).stripWhiteSpace();
                email=myChild->text(1);
                KABC::Addressee::List addressees = ab->findByEmail( email );
                if (addressees.count()==1) {
                        a=addressees.first();
                        KgpgInterface *ks=new KgpgInterface();
                        key.setTextData(ks->getKey(myChild->text(6),true));
                        a.insertKey(key);
                        ab->insertAddressee(a);
                        keylist<<myChild->text(6)+": "+email;
                }
                //            doSomething( myChild );
                myChild = myChild->nextSibling();
        }
        KABC::StdAddressBook::save();
        if (!keylist.isEmpty())
                KMessageBox::informationList(this,i18n("The following keys were exported to the address book:"),keylist);
        else
                KMessageBox::sorry(this,i18n("No entry matching your keys were found in the address book."));
}
*/

void listKeys::slotManpage()
{
        kapp->startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void listKeys::slotTip()
{
        KTipDialog::showTip(this, QString("kgpg/tips"), true);
}

void listKeys::closeEvent ( QCloseEvent * e )
{
        //kapp->ref(); // prevent KMainWindow from closing the app
        //KMainWindow::closeEvent( e );
        e->accept();
        //	hide();
        //	e->ignore();
}

void listKeys::showKeyServer()
{
        keyServer *ks=new keyServer(this);
	connect(ks,SIGNAL( importFinished(QString) ) , keysList2, SLOT(refreshcurrentkey(QString)));
        ks->exec();
	if (ks)
                delete ks;
        refreshkey();
}


void listKeys::checkList()
{
        QPtrList<QListViewItem> exportList=keysList2->selectedItems();
        if (exportList.count()>1)
                {
		stateChanged("multi_selected");
		for ( uint i = 0; i < exportList.count(); ++i )
		{
		    if (exportList.at(i) && !(exportList.at(i)->isVisible()))
                        exportList.at(i)->setSelected(false);
		}
		}
        else {
                if (keysList2->currentItem()->text(6).isEmpty())
                        stateChanged("group_selected");
                else
		  stateChanged("single_selected");

        }
        int serial=keysList2->currentItem()->pixmap(0)->serialNumber();
        if (serial==keysList2->pixkeySingle.serialNumber()) {
                if (keysList2->currentItem()->depth()==0)
                        changeMessage(i18n("Public Key"),0);
                else
                        changeMessage(i18n("Sub Key"),0);
        } else if (serial==keysList2->pixkeyPair.serialNumber())
                changeMessage(i18n("Secret Key Pair"),0);
        else if (serial==keysList2->pixkeyGroup.serialNumber())
                changeMessage(i18n("Key Group"),0);
        else if (serial==keysList2->pixsignature.serialNumber())
                changeMessage(i18n("Signature"),0);
        else if (serial==keysList2->pixuserid.serialNumber())
                changeMessage(i18n("User ID"),0);
        else if (keysList2->currentItem()->text(0)==i18n("Photo id"))
                changeMessage(i18n("Photo ID"),0);
        else if (serial==keysList2->pixRevoke.serialNumber())
                changeMessage(i18n("Revocation Signature"),0);
        else if (serial==keysList2->pixkeyOrphan.serialNumber())
                changeMessage(i18n("Orphaned Secret Key"),0);

}

void listKeys::annule()
{
        /////////  close window
        close();
}

void listKeys::quitApp()
{
        /////////  close window
        exit(1);
}

void listKeys::readOptions()
{

        clipboardMode=QClipboard::Clipboard;
        if (KGpgSettings::useMouseSelection() && (kapp->clipboard()->supportsSelection()))
                clipboardMode=QClipboard::Selection;

        ///////  re-read groups in case the config file location was changed
        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
        KGpgSettings::setGroups(groups.join(","));
        keysList2->groupNb=groups.count();
        if (keyStatusBar)
                changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb),1);

        showTipOfDay= KGpgSettings::showTipOfDay();
}


void listKeys::showOptions()
{
        if (KConfigDialog::showDialog("settings"))
                return;
        kgpgOptions *optionsDialog=new kgpgOptions(this,"settings");
        connect(optionsDialog,SIGNAL(settingsUpdated()),this,SLOT(readAllOptions()));
        connect(optionsDialog,SIGNAL(homeChanged()),this,SLOT(refreshkey()));
	connect(optionsDialog,SIGNAL(reloadKeyList()),this,SLOT(refreshkey()));
	connect(optionsDialog,SIGNAL(refreshTrust(int,QColor)),keysList2,SLOT(refreshTrust(int,QColor)));
        connect(optionsDialog,SIGNAL(changeFont(QFont)),this,SIGNAL(fontChanged(QFont)));
	connect(optionsDialog,SIGNAL(installShredder()),this,SIGNAL(installShredder()));
        optionsDialog->exec();
	delete optionsDialog;
}

void listKeys::readAllOptions()
{
        readOptions();
        emit readAgainOptions();
}

void listKeys::slotSetDefKey()
{
        slotSetDefaultKey(keysList2->currentItem());
}

void listKeys::slotSetDefaultKey(QString newID)
{
        QListViewItem *newdef = keysList2->findItem(newID,6);
        if (newdef)
                slotSetDefaultKey(newdef);
}

void listKeys::slotSetDefaultKey(QListViewItem *newdef)
{
        //kdDebug(2100)<<"------------------start ------------"<<endl;
        if ((!newdef) || (newdef->pixmap(2)==NULL))
                return;
        //kdDebug(2100)<<newdef->text(6)<<endl;
        //kdDebug(2100)<<KGpgSettings::defaultKey()<<endl;
        if (newdef->text(6)==KGpgSettings::defaultKey())
                return;
        if (newdef->pixmap(2)->serialNumber()!=keysList2->trustgood.serialNumber()) {
                KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
                return;
        }

        QListViewItem *olddef = keysList2->findItem(KGpgSettings::defaultKey(),6);

        KGpgSettings::setDefaultKey(newdef->text(6));
        KGpgSettings::writeConfig();
        if (olddef)
                keysList2->refreshcurrentkey(olddef);
        keysList2->refreshcurrentkey(newdef);
        keysList2->ensureItemVisible(keysList2->currentItem());
}



void listKeys::slotmenu(QListViewItem *sel, const QPoint &pos, int )
{
        ////////////  popup a different menu depending on which key is selected
        if (sel!=NULL) {
                if (keysList2->selectedItems().count()>1) {
                        QPtrList<QListViewItem> exportList=keysList2->selectedItems();
                        bool keyDepth=true;
                        for ( uint i = 0; i < exportList.count(); ++i )
                                if ( exportList.at(i) )
                                        if (exportList.at(i)->depth()!=0)
                                                keyDepth=false;
                        if (!keyDepth) {
                                signKey->setEnabled(false);
                                refreshKey->setEnabled(false);
                                popupout->exec(pos);
                                return;
                        } else {
                                signKey->setEnabled(true);
                                refreshKey->setEnabled(true);
                        }
                }

                if (sel->depth()!=0) {
                        //kdDebug(2100)<<sel->text(0)<<endl;
                        if ((sel->text(4)=="-") && (sel->text(6).startsWith("0x"))) {
                                if ((sel->text(2)=="-") || (sel->text(2)==i18n("Revoked"))) {
                                        if ((sel->text(0).startsWith("[")) && (sel->text(0).endsWith("]")))  ////// ugly hack to detect unknown keys
                                                importSignatureKey->setEnabled(true);
                                        else
                                                importSignatureKey->setEnabled(false);
                                        popupsig->exec(pos);
                                        return;
                                }
                        } else if (sel->text(0)==i18n("Photo id"))
                                popupphoto->exec(pos);
                        else if (sel->text(6)==("-"))
                                popupuid->exec(pos);
                } else {
                        keysList2->setSelected(sel,TRUE);
                        if (keysList2->currentItem()->text(6).isEmpty())
                                popupgroup->exec(pos);
                        else {
                                if ((keysList2->secretList.find(sel->text(6))!=-1) && (keysList2->selectedItems().count()==1))
                                        popupsec->exec(pos);
                                else
                                        if ((keysList2->orphanList.find(sel->text(6))!=-1) && (keysList2->selectedItems().count()==1))
                                                popuporphan->exec(pos);
                                        else
                                                popup->exec(pos);
                        }
                        return;
                }
        } else
                popupout->exec(pos);
}



void listKeys::slotrevoke(QString keyID,QString revokeUrl,int reason,QString description)
{
        revKeyProcess=new KgpgInterface();
        revKeyProcess->KgpgRevokeKey(keyID,revokeUrl,reason,description);
}


void listKeys::revokeWidget()
{
        KDialogBase *keyRevokeWidget=new KDialogBase(KDialogBase::Swallow, i18n("Create Revocation Certificate"),  KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,this,0,true);

        KgpgRevokeWidget *keyRevoke=new KgpgRevokeWidget();

        keyRevoke->keyID->setText(keysList2->currentItem()->text(0)+" ("+keysList2->currentItem()->text(1)+") "+i18n("ID: ")+keysList2->currentItem()->text(6));
        keyRevoke->kURLRequester1->setURL(QDir::homeDirPath()+"/"+keysList2->currentItem()->text(1).section('@',0,0)+".revoke");
        keyRevoke->kURLRequester1->setMode(KFile::File);

        keyRevoke->setMinimumSize(keyRevoke->sizeHint());
        keyRevoke->show();
        keyRevokeWidget->setMainWidget(keyRevoke);

        if (keyRevokeWidget->exec()!=QDialog::Accepted)
                return;
        if (keyRevoke->cbSave->isChecked()) {
                slotrevoke(keysList2->currentItem()->text(6),keyRevoke->kURLRequester1->url(),keyRevoke->comboBox1->currentItem(),keyRevoke->textDescription->text());
                if (keyRevoke->cbPrint->isChecked())
                        connect(revKeyProcess,SIGNAL(revokeurl(QString)),this,SLOT(doFilePrint(QString)));
                if (keyRevoke->cbImport->isChecked())
                        connect(revKeyProcess,SIGNAL(revokeurl(QString)),this,SLOT(slotImportRevoke(QString)));
        } else {
                slotrevoke(keysList2->currentItem()->text(6),QString::null,keyRevoke->comboBox1->currentItem(),keyRevoke->textDescription->text());
                if (keyRevoke->cbPrint->isChecked())
                        connect(revKeyProcess,SIGNAL(revokecertificate(QString)),this,SLOT(doPrint(QString)));
                if (keyRevoke->cbImport->isChecked())
                        connect(revKeyProcess,SIGNAL(revokecertificate(QString)),this,SLOT(slotImportRevokeTxt(QString)));
        }
}


void listKeys::slotImportRevoke(QString url)
{
        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKeyURL(KURL::fromPathOrURL( url ));
        connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(refreshselfkey()));
}

void listKeys::slotImportRevokeTxt(QString revokeText)
{
        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKey(revokeText);
        connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(refreshselfkey()));
}

void listKeys::slotexportsec()
{
        //////////////////////   export secret key
        QString warn=i18n("Secret keys SHOULD NOT be saved in an unsafe place.\n"
                          "If someone else can access this file, encryption with this key will be compromised!\nContinue key export?");
        int result=KMessageBox::questionYesNo(this,warn,i18n("Warning"), i18n("Export"), i18n("Do Not Export"));
        if (result!=KMessageBox::Yes)
                return;

        QString sname=keysList2->currentItem()->text(1).section('@',0,0);
        sname=sname.section('.',0,0);
        if (sname.isEmpty())
                sname=keysList2->currentItem()->text(0).section(' ',0,0);
        sname.append(".asc");
        sname.prepend(QDir::homeDirPath()+"/");
        KURL url=KFileDialog::getSaveURL(sname,"*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

        if(!url.isEmpty()) {
                QFile fgpg(url.path());
                if (fgpg.exists())
                        fgpg.remove();

                KProcIO *p=new KProcIO(QTextCodec::codecForLocale());
                *p<<"gpg"<<"--no-tty"<<"--output"<<QFile::encodeName(url.path())<<"--armor"<<"--export-secret-keys"<<keysList2->currentItem()->text(6);
                p->start(KProcess::Block);

                if (fgpg.exists())
                        KMessageBox::information(this,i18n("Your PRIVATE key \"%1\" was successfully exported.\nDO NOT leave it in an insecure place.").arg(url.path()));
                else
                        KMessageBox::sorry(this,i18n("Your secret key could not be exported.\nCheck the key."));
        }

}


void listKeys::slotexport()
{
        /////////////////////  export key
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0)
                return;


        QPtrList<QListViewItem> exportList=keysList2->selectedItems();
        if (exportList.count()==0)
                return;

        QString sname;

        if (exportList.count()==1) {
                sname=keysList2->currentItem()->text(1).section('@',0,0);
                sname=sname.section('.',0,0);
                if (sname.isEmpty())
                        sname=keysList2->currentItem()->text(0).section(' ',0,0);
        } else
                sname="keyring";
        sname.append(".asc");
        sname.prepend(QDir::homeDirPath()+"/");

        KDialogBase *dial=new KDialogBase( KDialogBase::Swallow, i18n("Public Key Export"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "key_export",true);

        KeyExport *page=new KeyExport();
        dial->setMainWidget(page);
        page->newFilename->setURL(sname);
        page->newFilename->setCaption(i18n("Save File"));
        page->newFilename->setMode(KFile::File);
        page->show();

        if (dial->exec()==QDialog::Accepted) {
                ////////////////////////// export to file
                QString expname;
                bool exportAttr=page->exportAttributes->isChecked();
                if (page->checkServer->isChecked()) {
                        keyServer *expServer=new keyServer(0,"server_export",false);
                        expServer->page->exportAttributes->setChecked(exportAttr);
                        QStringList exportKeysList;
                        for ( uint i = 0; i < exportList.count(); ++i )
                                if ( exportList.at(i) )
                                        exportKeysList << exportList.at(i)->text(6).stripWhiteSpace();
                        expServer->slotExport(exportKeysList);
                        return;
                }
                KProcIO *p=new KProcIO(QTextCodec::codecForLocale());
                *p<<"gpg"<<"--no-tty";
                if (page->checkFile->isChecked()) {
                        expname=page->newFilename->url().stripWhiteSpace();
                        if (!expname.isEmpty()) {
                                QFile fgpg(expname);
                                if (fgpg.exists())
                                        fgpg.remove();
                                *p<<"--output"<<QFile::encodeName(expname)<<"--export"<<"--armor";
                                if (!exportAttr)
                                        *p<<"--export-options"<<"no-include-attributes";

                                for ( uint i = 0; i < exportList.count(); ++i )
                                        if ( exportList.at(i) )
                                                *p<<(exportList.at(i)->text(6)).stripWhiteSpace();


                                p->start(KProcess::Block);
                                if (fgpg.exists())
                                        KMessageBox::information(this,i18n("Your public key \"%1\" was successfully exported\n").arg(expname));
                                else
                                        KMessageBox::sorry(this,i18n("Your public key could not be exported\nCheck the key."));
                        }
                } else {

                        QStringList klist;

                        for ( uint i = 0; i < exportList.count(); ++i )
                                if ( exportList.at(i) )
                                        klist.append(exportList.at(i)->text(6).stripWhiteSpace());

                        KgpgInterface *kexp=new KgpgInterface();

                        QString result=kexp->getKey(klist,exportAttr);
                        if (page->checkClipboard->isChecked())
                                slotProcessExportClip(result);
                        //connect(kexp,SIGNAL(publicKeyString(QString)),this,SLOT(slotProcessExportClip(QString)));
                        else
                                slotProcessExportMail(result);
                        //connect(kexp,SIGNAL(publicKeyString(QString)),this,SLOT(slotProcessExportMail(QString)));

                }
        }
        delete dial;
}



void listKeys::slotProcessExportMail(QString keys)
{
        ///   start default Mail application
        kapp->invokeMailer(QString::null, QString::null, QString::null, QString::null,
                           keys, //body
                           QString::null,
                           QString::null); // attachments
}

void listKeys::slotProcessExportClip(QString keys)
{
        kapp->clipboard()->setText(keys,clipboardMode);
}


void listKeys::showKeyInfo(QString keyID)
{
        KgpgKeyInfo *opts=new KgpgKeyInfo(this,"key_props",keyID);
        opts->show();
}


void listKeys::slotShowPhoto()
{
        KTrader::OfferList offers = KTrader::self()->query("image/jpeg", "Type == 'Application'");
        KService::Ptr ptr = offers.first();
        //KMessageBox::sorry(0,ptr->desktopEntryName());
        KProcIO *p=new KProcIO(QTextCodec::codecForLocale());
        *p<<"gpg"<<"--no-tty"<<"--photo-viewer"<<QFile::encodeName(ptr->desktopEntryName()+" %i")<<"--edit-key"<<keysList2->currentItem()->parent()->text(6)<<"uid"<<keysList2->currentItem()->text(6)<<"showphoto"<<"quit";
        p->start(KProcess::DontCare,true);
}

void listKeys::listsigns()
{
        //kdDebug(2100)<<"Edit -------------------------------"<<endl;
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0) {
                if (keysList2->currentItem()->text(0)==i18n("Photo id")) {
                        //////////////////////////    display photo
                        slotShowPhoto();
                }
                return;
        }

        if (keysList2->currentItem()->pixmap(0)->serialNumber()==keysList2->pixkeyOrphan.serialNumber()) {
                if (KMessageBox::questionYesNo(this,i18n("This key is an orphaned secret key (secret key without public key.) It is currently not usable.\n\n"
                                               "Would you like to regenerate the public key?"), QString::null, i18n("Generate"), i18n("Do Not Generate"))==KMessageBox::Yes)
                        slotregenerate();
                return;
        }

        /////////////   open a key info dialog (KgpgKeyInfo, see begining of this file)
        QString key=keysList2->currentItem()->text(6);
        if (!key.isEmpty()) {
                KgpgKeyInfo *opts=new KgpgKeyInfo(this,"key_props",key);
                connect(opts,SIGNAL(keyNeedsRefresh()),keysList2,SLOT(refreshselfkey()));
                opts->exec();
        } else
                editGroup();
}

void listKeys::groupAdd()
{
        QPtrList<QListViewItem> addList=gEdit->availableKeys->selectedItems();
        for ( uint i = 0; i < addList.count(); ++i )
                if ( addList.at(i) ) {
                        gEdit->groupKeys->insertItem(addList.at(i));
                }
}

void listKeys::groupRemove()
{
        QPtrList<QListViewItem> remList=gEdit->groupKeys->selectedItems();
        for ( uint i = 0; i < remList.count(); ++i )
                if ( remList.at(i) ) {
                        gEdit->availableKeys->insertItem(remList.at(i));
                }
}

void listKeys::deleteGroup()
{
        if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
                return;

        int result=KMessageBox::warningContinueCancel(this,i18n("<qt>Are you sure you want to delete group <b>%1</b> ?</qt>").arg(keysList2->currentItem()->text(0)),i18n("Warning"),KGuiItem(i18n("Delete"),"editdelete"));
        if (result!=KMessageBox::Continue)
                return;
        KgpgInterface::delGpgGroup(keysList2->currentItem()->text(0), KGpgSettings::gpgConfigPath());
        QListViewItem *item=keysList2->currentItem()->nextSibling();
        delete keysList2->currentItem();
        if (!item)
                item=keysList2->lastChild();
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item,true);

        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
        KGpgSettings::setGroups(groups.join(","));
        keysList2->groupNb=groups.count();
        changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb),1);
}

void listKeys::groupChange()
{
        QStringList selected;
        QListViewItem *item=gEdit->groupKeys->firstChild();
        while (item) {
                selected+=item->text(2);
                item=item->nextSibling();
        }
        KgpgInterface::setGpgGroupSetting(keysList2->currentItem()->text(0),selected,KGpgSettings::gpgConfigPath());
}

void listKeys::createNewGroup()
{
        QStringList badkeys,keysGroup;

        if (keysList2->selectedItems().count()>0) {
                QPtrList<QListViewItem> groupList=keysList2->selectedItems();
                bool keyDepth=true;
                for ( uint i = 0; i < groupList.count(); ++i )
                        if ( groupList.at(i) ) {
                                if (groupList.at(i)->depth()!=0)
                                        keyDepth=false;
                                else if (groupList.at(i)->text(6).isEmpty())
                                        keyDepth=false;
                                else if (groupList.at(i)->pixmap(2)) {
                                        if (groupList.at(i)->pixmap(2)->serialNumber()==keysList2->trustgood.serialNumber())
                                                keysGroup+=groupList.at(i)->text(6);
                                        else
                                                badkeys+=groupList.at(i)->text(0)+" ("+groupList.at(i)->text(1)+") "+groupList.at(i)->text(6);
                                }

                        }
                if (!keyDepth) {
                        KMessageBox::sorry(this,i18n("<qt>You cannot create a group containing signatures, subkeys or other groups.</qt>"));
                        return;
                }
                QString groupName=KInputDialog::getText(i18n("Create New Group"),i18n("Enter new group name:"),QString::null,0,this);
                if (groupName.isEmpty())
                        return;
                if (!keysGroup.isEmpty()) {
                        if (!badkeys.isEmpty())
                                KMessageBox::informationList(this,i18n("Following keys are not valid or not trusted and will not be added to the group:"),badkeys);
                        KgpgInterface::setGpgGroupSetting(groupName,keysGroup,KGpgSettings::gpgConfigPath());
                        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
                        KGpgSettings::setGroups(groups.join(","));
                        keysList2->refreshgroups();
                        QListViewItem *newgrp = keysList2->findItem(groupName,0);

                        keysList2->clearSelection();
                        keysList2->setCurrentItem(newgrp);
                        keysList2->setSelected(newgrp,true);
                        keysList2->ensureItemVisible(newgrp);
                        keysList2->groupNb=groups.count();
                        changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb),1);
                } else
                        KMessageBox::sorry(this,i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>").arg(groupName));
        }
}

void listKeys::groupInit(QStringList keysGroup)
{
        kdDebug(2100)<<"preparing group"<<endl;
        QStringList lostKeys;
        bool foundId;

        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {

                QListViewItem *item=gEdit->availableKeys->firstChild();
                foundId=false;
                while (item) {
                        kdDebug(2100)<<"Searching in key: "<<item->text(0)<<endl;
                        if (QString(*it).right(8).lower()==item->text(2).right(8).lower()) {
                                gEdit->groupKeys->insertItem(item);
                                foundId=true;
                                break;
                        }
                        item=item->nextSibling();
                }
                if (!foundId)
                        lostKeys+=QString(*it);
        }
        if (!lostKeys.isEmpty())
                KMessageBox::informationList(this,i18n("Following keys are in the group but are not valid or not in your keyring. They will be removed from the group."),lostKeys);
}

void listKeys::editGroup()
{
  if (!keysList2->currentItem() || !keysList2->currentItem()->text(6).isEmpty())
                return;
        QStringList keysGroup;
	//KDialogBase *dialogGroupEdit=new KDialogBase( this, "edit_group", true,i18n("Group Properties"),KDialogBase::Ok | KDialogBase::Cancel);
        KDialogBase *dialogGroupEdit=new KDialogBase(KDialogBase::Swallow, i18n("Group Properties"), KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,this,0,true);

        gEdit=new groupEdit();
        gEdit->buttonAdd->setPixmap(KGlobal::iconLoader()->loadIcon("down",KIcon::Small,20));
        gEdit->buttonRemove->setPixmap(KGlobal::iconLoader()->loadIcon("up",KIcon::Small,20));

        connect(gEdit->buttonAdd,SIGNAL(clicked()),this,SLOT(groupAdd()));
        connect(gEdit->buttonRemove,SIGNAL(clicked()),this,SLOT(groupRemove()));
        //        connect(dialogGroupEdit->okClicked(),SIGNAL(clicked()),this,SLOT(groupChange()));
        connect(gEdit->availableKeys,SIGNAL(doubleClicked (QListViewItem *, const QPoint &, int)),this,SLOT(groupAdd()));
        connect(gEdit->groupKeys,SIGNAL(doubleClicked (QListViewItem *, const QPoint &, int)),this,SLOT(groupRemove()));
        QListViewItem *item=keysList2->firstChild();
        if (item==NULL)
                return;
        if (item->pixmap(2)) {
                if (item->pixmap(2)->serialNumber()==keysList2->trustgood.serialNumber())
                        (void) new KListViewItem(gEdit->availableKeys,item->text(0),item->text(1),item->text(6));
        }
        while (item->nextSibling()) {
                item=item->nextSibling();
                if (item->pixmap(2)) {
                        if (item->pixmap(2)->serialNumber()==keysList2->trustgood.serialNumber())
                                (void) new KListViewItem(gEdit->availableKeys,item->text(0),item->text(1),item->text(6));
                }
        }
        keysGroup=KgpgInterface::getGpgGroupSetting(keysList2->currentItem()->text(0),KGpgSettings::gpgConfigPath());
        groupInit(keysGroup);
	dialogGroupEdit->setMainWidget(gEdit);
	gEdit->availableKeys->setColumnWidth(0,200);
	gEdit->availableKeys->setColumnWidth(1,200);
	gEdit->availableKeys->setColumnWidth(2,100);
	gEdit->availableKeys->setColumnWidthMode(0,QListView::Manual);
	gEdit->availableKeys->setColumnWidthMode(1,QListView::Manual);
	gEdit->availableKeys->setColumnWidthMode(2,QListView::Manual);

	gEdit->groupKeys->setColumnWidth(0,200);
	gEdit->groupKeys->setColumnWidth(1,200);
	gEdit->groupKeys->setColumnWidth(2,100);
	gEdit->groupKeys->setColumnWidthMode(0,QListView::Manual);
	gEdit->groupKeys->setColumnWidthMode(1,QListView::Manual);
	gEdit->groupKeys->setColumnWidthMode(2,QListView::Manual);

        gEdit->setMinimumSize(gEdit->sizeHint());
        gEdit->show();
        if (dialogGroupEdit->exec()==QDialog::Accepted)
                groupChange();
        delete dialogGroupEdit;
}

void listKeys::signkey()
{
        ///////////////  sign a key
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0)
                return;

        signList=keysList2->selectedItems();
        bool keyDepth=true;
        for ( uint i = 0; i < signList.count(); ++i )
                if ( signList.at(i) )
                        if (signList.at(i)->depth()!=0)
                                keyDepth=false;
        if (!keyDepth) {
                KMessageBox::sorry(this,i18n("You can only sign primary keys. Please check your selection."));
                return;
        }


        if (signList.count()==1) {
                FILE *pass;
                char line[200]="";
                QString opt,fingervalue;
                QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colons --fingerprint "+KShellProcess::quote(keysList2->currentItem()->text(6));
                pass=popen(QFile::encodeName(gpgcmd),"r");
                while ( fgets( line, sizeof(line), pass)) {
                        opt=QString::fromUtf8(line);
                        if (opt.startsWith("fpr")) {
                                fingervalue=opt.section(':',9,9);
                                // format fingervalue in 4-digit groups
                                uint len = fingervalue.length();
                                if ((len > 0) && (len % 4 == 0))
                                        for (uint n = 0; 4*(n+1) < len; n++)
                                                fingervalue.insert(5*n+4, ' ');
                        }
                }
                pclose(pass);
                opt=	i18n("<qt>You are about to sign key:<br><br>%1<br>ID: %2<br>Fingerprint: <br><b>%3</b>.<br><br>"
                          "You should check the key fingerprint by phoning or meeting the key owner to be sure that someone "
                          "is not trying to intercept your communications</qt>").arg(keysList2->currentItem()->text(0)+" ("+keysList2->currentItem()->text(1)+")").arg(keysList2->currentItem()->text(6)).arg(fingervalue);

                if (KMessageBox::warningContinueCancel(this,opt)!=KMessageBox::Continue)
                        return;

        } else {
                QStringList signKeyList;
                for ( uint i = 0; i < signList.count(); ++i )
                        if ( signList.at(i) )
                                signKeyList+=signList.at(i)->text(0)+" ("+signList.at(i)->text(1)+")"+": "+signList.at(i)->text(6);
                if (KMessageBox::warningContinueCancelList(this,i18n("<qt>You are about to sign the following keys in one pass.<br><b>If you have not carefully checked all fingerprints, the security of your communications may be compromised.</b></qt>"),signKeyList)!=KMessageBox::Continue)
                        return;
        }


        //////////////////  open a secret key selection dialog (KgpgSelKey, see begining of this file)
        KgpgSelKey *opts=new KgpgSelKey(this);

        QLabel *signCheck = new QLabel("<qt>"+i18n("How carefully have you checked that the key really "
                                            "belongs to the person with whom you wish to communicate:",
					    "How carefully have you checked that the %n keys really "
                                            "belong to the people with whom you wish to communicate:",signList.count()),opts->page);
        opts->vbox->addWidget(signCheck);
        QComboBox *signTrust=new QComboBox(opts->page);
        signTrust->insertItem(i18n("I Will Not Answer"));
        signTrust->insertItem(i18n("I Have Not Checked at All"));
        signTrust->insertItem(i18n("I Have Done Casual Checking"));
        signTrust->insertItem(i18n("I Have Done Very Careful Checking"));
        opts->vbox->addWidget(signTrust);

        QCheckBox *localSign = new QCheckBox(i18n("Local signature (cannot be exported)"),opts->page);
        opts->vbox->addWidget(localSign);

        QCheckBox *terminalSign = new QCheckBox(i18n("Do not sign all user id's (open terminal)"),opts->page);
        opts->vbox->addWidget(terminalSign);
        if (signList.count()!=1)
                terminalSign->setEnabled(false);

        opts->setMinimumHeight(300);

        if (opts->exec()!=QDialog::Accepted) {
                delete opts;
                return;
        }

        globalkeyID=QString(opts->getkeyID());
        globalkeyMail=QString(opts->getkeyMail());
        globalisLocal=localSign->isChecked();
        globalChecked=signTrust->currentItem();
        keyCount=0;
        delete opts;
        globalCount=signList.count();
        if (!terminalSign->isChecked())
                signLoop();
        else {
                KProcess kp;

                KConfig *config = KGlobal::config();
                config->setGroup("General");
                kp<< config->readPathEntry("TerminalApplication","konsole");
                kp<<"-e"
                <<"gpg"
                <<"--no-secmem-warning"
                <<"-u"
                <<globalkeyID
                <<"--edit-key"
                <<signList.at(0)->text(6);
                if (globalisLocal)
                        kp<<"lsign";
                else
                        kp<<"sign";
                kp.start(KProcess::Block);
                keysList2->refreshcurrentkey(keysList2->currentItem());
        }
}

void listKeys::signLoop()
{
        if (keyCount<globalCount) {
                kdDebug(2100)<<"Sign process for key: "<<keyCount<<" on a total of "<<signList.count()<<endl;
                if ( signList.at(keyCount) ) {
                        KgpgInterface *signKeyProcess=new KgpgInterface();
			QObject::connect(signKeyProcess,SIGNAL(signatureFinished(int)),this,SLOT(signatureResult(int)));
                        signKeyProcess->KgpgSignKey(signList.at(keyCount)->text(6),globalkeyID,globalkeyMail,globalisLocal,globalChecked);
                }
        }
}

void listKeys::signatureResult(int success)
{
        if (success==3)
                keysList2->refreshcurrentkey(signList.at(keyCount));

        else if (success==2)
                KMessageBox::sorry(this,i18n("<qt>Bad passphrase, key <b>%1</b> not signed.</qt>").arg(signList.at(keyCount)->text(0)+i18n(" (")+signList.at(keyCount)->text(1)+i18n(")")));

        keyCount++;
        signLoop();
}


void listKeys::importallsignkey()
{
        if (keysList2->currentItem()==NULL)
                return;
        if (! keysList2->currentItem()->firstChild()) {
                keysList2->currentItem()->setOpen(true);
                keysList2->currentItem()->setOpen(false);
        }
        QString missingKeysList;
        QListViewItem *current = keysList2->currentItem()->firstChild();
        while (current) {
                if ((current->text(0).startsWith("[")) && (current->text(0).endsWith("]")))   ////// ugly hack to detect unknown keys
                        missingKeysList+=current->text(6)+" ";
                current = current->nextSibling();
        }
        if (!missingKeysList.isEmpty())
                importsignkey(missingKeysList);
        else
                KMessageBox::information(this,i18n("All signatures for this key are already in your keyring"));
}


void listKeys::preimportsignkey()
{
        if (keysList2->currentItem()==NULL)
                return;
        else
                importsignkey(keysList2->currentItem()->text(6));
}

bool listKeys::importRemoteKey(QString keyID)
{

        kServer=new keyServer(0,"server_dialog",false,true);
        kServer->page->kLEimportid->setText(keyID);
        kServer->page->Buttonimport->setDefault(true);
        kServer->page->tabWidget2->setTabEnabled(kServer->page->tabWidget2->page(1),false);
        kServer->show();
	kServer->raise();
        connect( kServer, SIGNAL( importFinished(QString) ) , this, SLOT( dcopImportFinished()));

	return true;
}



void listKeys::dcopImportFinished()
{
        if (kServer)
                kServer=0L;
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
   stream << true;
    kapp->dcopClient()->emitDCOPSignal("keyImported(bool)", params);
    refreshkey();
}

void listKeys::importsignkey(QString importKeyId)
{
        ///////////////  sign a key
        kServer=new keyServer(0,"server_dialog",false);
        kServer->page->kLEimportid->setText(importKeyId);
        //kServer->Buttonimport->setDefault(true);
        kServer->slotImport();
        //kServer->show();
        connect( kServer, SIGNAL( importFinished(QString) ) , this, SLOT( importfinished()));
}


void listKeys::importfinished()
{
        if (kServer)
                kServer=0L;
        refreshkey();
}


void listKeys::delsignkey()
{
        ///////////////  sign a key
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()>1) {
                KMessageBox::sorry(this,i18n("Edit key manually to delete this signature."));
                return;
        }

        QString signID,parentKey,signMail,parentMail;

        //////////////////  open a key selection dialog (KgpgSelKey, see begining of this file)
        parentKey=keysList2->currentItem()->parent()->text(6);
        signID=keysList2->currentItem()->text(6);
        parentMail=keysList2->currentItem()->parent()->text(0)+" ("+keysList2->currentItem()->parent()->text(1)+")";
        signMail=keysList2->currentItem()->text(0)+" ("+keysList2->currentItem()->text(1)+")";

        if (parentKey==signID) {
                KMessageBox::sorry(this,i18n("Edit key manually to delete a self-signature."));
                return;
        }
        QString ask=i18n("<qt>Are you sure you want to delete signature<br><b>%1</b> from key:<br><b>%2</b>?</qt>").arg(signMail).arg(parentMail);

        if (KMessageBox::questionYesNo(this,ask,QString::null,KStdGuiItem::del(),KStdGuiItem::cancel())!=KMessageBox::Yes)
                return;
        KgpgInterface *delSignKeyProcess=new KgpgInterface();
        delSignKeyProcess->KgpgDelSignature(parentKey,signID);
        connect(delSignKeyProcess,SIGNAL(delsigfinished(bool)),this,SLOT(delsignatureResult(bool)));
}

void listKeys::delsignatureResult(bool success)
{
        if (success) {
                QListViewItem *top=keysList2->currentItem();
                while (top->depth()!=0)
                        top=top->parent();
                while (top->firstChild()!=0)
                        delete top->firstChild();
                keysList2->refreshcurrentkey(top);
        } else
                KMessageBox::sorry(this,i18n("Requested operation was unsuccessful, please edit the key manually."));
}

void listKeys::slotedit()
{
        if (!keysList2->currentItem())
                return;
        if (keysList2->currentItem()->depth()!=0)
                return;
        if (keysList2->currentItem()->text(6).isEmpty())
                return;

        KProcess kp;

        KConfig *config = KGlobal::config();
        config->setGroup("General");
        kp<< config->readPathEntry("TerminalApplication","konsole");
        kp<<"-e"
        <<"gpg"
        <<"--no-secmem-warning"
        <<"--utf8-strings"
        <<"--edit-key"
        <<keysList2->currentItem()->text(6)
        <<"help";
        kp.start(KProcess::Block);
        keysList2->refreshcurrentkey(keysList2->currentItem());
}


void listKeys::slotgenkey()
{
        //////////  generate key
        keyGenerate *genkey=new keyGenerate(this,0);
        if (genkey->exec()==QDialog::Accepted) {
                if (!genkey->getmode())   ///  normal mode
                {
                        //// extract data
                        QString ktype=genkey->getkeytype();
                        QString ksize=genkey->getkeysize();
                        int kexp=genkey->getkeyexp();
                        QString knumb=genkey->getkeynumb();
                        newKeyName=genkey->getkeyname();
                        newKeyMail=genkey->getkeymail();
                        QString kcomment=genkey->getkeycomm();
                        delete genkey;

                        //genkey->delayedDestruct();
                        QCString password;
                        bool goodpass=false;
                        while (!goodpass)
                        {
                                int code=KPasswordDialog::getNewPassword(password,i18n("<b>Enter passphrase for %1</b>:<br>Passphrase should include non alphanumeric characters and random sequences").arg(newKeyName+" <"+newKeyMail+">"));
                                if (code!=QDialog::Accepted)
                                        return;
                                if (password.length()<5)
                                        KMessageBox::sorry(this,i18n("This passphrase is not secure enough.\nMinimum length= 5 characters"));
                                else
                                        goodpass=true;
                        }

			pop = new KPassivePopup((QWidget *)parent(),"new_key");
                        pop->setTimeout(0);

                        QWidget *wid=new QWidget(pop);
                        QVBoxLayout *vbox=new QVBoxLayout(wid,3);

                        QVBox *passiveBox=pop->standardView(i18n("Generating new key pair."),QString::null,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),wid);


                        QMovie anim;
                        anim=QMovie(locate("appdata", "pics/kgpg_anim.gif"));

                        QLabel *tex=new QLabel(wid);
                        QLabel *tex2=new QLabel(wid);
                        tex->setAlignment(AlignHCenter);
                        tex->setMovie(anim);
                        tex2->setText(i18n("\nPlease wait..."));
                        vbox->addWidget(passiveBox);
                        vbox->addWidget(tex);
                        vbox->addWidget(tex2);

                        pop->setView(wid);

                        pop->show();
                        changeMessage(i18n("Generating New Key..."),0,true);

                        QRect qRect(QApplication::desktop()->screenGeometry());
                        int iXpos=qRect.width()/2-pop->width()/2;
                        int iYpos=qRect.height()/2-pop->height()/2;
                        pop->move(iXpos,iYpos);
                        pop->setAutoDelete(false);
                        KProcIO *proc=new KProcIO(QTextCodec::codecForLocale());
                        message=QString::null;
                        //*proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--passphrase-fd"<<res<<"--gen-key"<<"-a"<<"kgpg.tmp";
                        *proc<<"gpg"<<"--no-tty"<<"--status-fd=2"<<"--no-secmem-warning"<<"--batch"<<"--gen-key"<<"--utf8-strings";
                        /////////  when process ends, update dialog infos
                        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this, SLOT(genover(KProcess *)));
                        proc->start(KProcess::NotifyOnExit,true);

                        if (ktype=="RSA")
                                proc->writeStdin("Key-Type: 1");
                        else
                        {
                                proc->writeStdin("Key-Type: DSA");
                                proc->writeStdin("Subkey-Type: ELG-E");
                                proc->writeStdin(QString("Subkey-Length:%1").arg(ksize));
                        }
                        proc->writeStdin(QString("Passphrase:%1").arg(password));
                        proc->writeStdin(QString("Key-Length:%1").arg(ksize));
                        proc->writeStdin(QString("Name-Real:%1").arg(newKeyName));
                        if (!newKeyMail.isEmpty())
                                proc->writeStdin(QString("Name-Email:%1").arg(newKeyMail));
                        if (!kcomment.isEmpty())
                                proc->writeStdin(QString("Name-Comment:%1").arg(kcomment));
                        if (kexp==0)
                                proc->writeStdin(QString("Expire-Date:0"));
                        if (kexp==1)
                                proc->writeStdin(QString("Expire-Date:%1").arg(knumb));
                        if (kexp==2)
                                proc->writeStdin(QString("Expire-Date:%1w").arg(knumb));

                        if (kexp==3)
                                proc->writeStdin(QString("Expire-Date:%1m").arg(knumb));

                        if (kexp==4)
                                proc->writeStdin(QString("Expire-Date:%1y").arg(knumb));
                        proc->writeStdin("%commit");
                        QObject::connect(proc,SIGNAL(readReady(KProcIO *)),this,SLOT(readgenprocess(KProcIO *)));
                        proc->closeWhenDone();
                } else  ////// start expert (=konsole) mode
                {
                        KProcess kp;

                        KConfig *config = KGlobal::config();
                        config->setGroup("General");
                        kp<< config->readPathEntry("TerminalApplication","konsole");
                        kp<<"-e"
                        <<"gpg"
                        <<"--gen-key";
                        kp.start(KProcess::Block);
                        refreshkey();
                }
        }
}

void listKeys::readgenprocess(KProcIO *p)
{
        QString required;
        while (p->readln(required,true)!=-1) {
                if (required.find("KEY_CREATED")!=-1)
                        newkeyFinger=required.stripWhiteSpace().section(' ',-1);
                message+=required+"\n";
        }

        //  sample:   [GNUPG:] KEY_CREATED B 156A4305085A58C01E2988229282910254D1B368
}

void listKeys::genover(KProcess *)
{
        newkeyID=QString::null;
        continueSearch=true;
        KProcIO *conprocess=new KProcIO(QTextCodec::codecForLocale());
        *conprocess<< "gpg";
        *conprocess<<"--no-secmem-warning"<<"--with-colons"<<"--fingerprint"<<"--list-keys"<<newKeyName;
        QObject::connect(conprocess,SIGNAL(readReady(KProcIO *)),this,SLOT(slotReadFingerProcess(KProcIO *)));
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(newKeyDone(KProcess *)));
        conprocess->start(KProcess::NotifyOnExit,true);
}


void listKeys::slotReadFingerProcess(KProcIO *p)
{
        QString outp;
        while (p->readln(outp)!=-1) {
                if (outp.startsWith("pub") && (continueSearch)) {
                        newkeyID=outp.section(':',4,4).right(8).prepend("0x");

                }
                if (outp.startsWith("fpr")) {
                        if (newkeyFinger.lower()==outp.section(':',9,9).lower())
                                continueSearch=false;
                        //			kdDebug(2100)<<newkeyFinger<<" test:"<<outp.section(':',9,9)<<endl;
                }
        }
}


void listKeys::newKeyDone(KProcess *)
{
        changeMessage(i18n("Ready"),0);
        //        refreshkey();
        if (newkeyID.isEmpty()) {
                delete pop;
                KMessageBox::detailedSorry(this,i18n("Something unexpected happened during the key pair creation.\nPlease check details for full log output."),message);
                refreshkey();
                return;
        }
        keysList2->refreshcurrentkey(newkeyID);
        changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb),1);
        KDialogBase *keyCreated=new KDialogBase( this, "key_created", true,i18n("New Key Pair Created"), KDialogBase::Ok);
        newKey *page=new newKey(keyCreated);
        page->TLname->setText("<b>"+newKeyName+"</b>");
        page->TLemail->setText("<b>"+newKeyMail+"</b>");
	if (!newKeyMail.isEmpty())
	page->kURLRequester1->setURL(QDir::homeDirPath()+"/"+newKeyMail.section("@",0,0)+".revoke");
	else
	page->kURLRequester1->setURL(QDir::homeDirPath()+"/"+newKeyName.section(" ",0,0)+".revoke");
        page->TLid->setText("<b>"+newkeyID+"</b>");
        page->LEfinger->setText(newkeyFinger);
        page->CBdefault->setChecked(true);
        page->show();
        //page->resize(page->maximumSize());
        keyCreated->setMainWidget(page);
        delete pop;
        keyCreated->exec();

        QListViewItem *newdef = keysList2->findItem(newkeyID,6);
        if (newdef)
                if (page->CBdefault->isChecked())
                        slotSetDefaultKey(newdef);
                else {
                        keysList2->clearSelection();
                        keysList2->setCurrentItem(newdef);
                        keysList2->setSelected(newdef,true);
                        keysList2->ensureItemVisible(newdef);
                }
        if (page->CBsave->isChecked()) {
                slotrevoke(newkeyID,page->kURLRequester1->url(),0,i18n("backup copy"));
                if (page->CBprint->isChecked())
                        connect(revKeyProcess,SIGNAL(revokeurl(QString)),this,SLOT(doFilePrint(QString)));
        } else if (page->CBprint->isChecked()) {
                slotrevoke(newkeyID,QString::null,0,i18n("backup copy"));
                connect(revKeyProcess,SIGNAL(revokecertificate(QString)),this,SLOT(doPrint(QString)));
        }
}

void listKeys::doFilePrint(QString url)
{
        QFile qfile(url);
        if (qfile.open(IO_ReadOnly)) {
                QTextStream t( &qfile );
                doPrint(t.read());
        } else
                KMessageBox::sorry(this,i18n("<qt>Cannot open file <b>%1</b> for printing...</qt>").arg(url));
}

void listKeys::doPrint(QString txt)
{
        KPrinter prt;
        //kdDebug(2100)<<"Printing..."<<endl;
        if (prt.setup(this)) {
                QPainter painter(&prt);
                QPaintDeviceMetrics metrics(painter.device());
                painter.drawText( 0, 0, metrics.width(), metrics.height(), AlignLeft|AlignTop|DontClip,txt );
        }
}

void listKeys::deleteseckey()
{
        //////////////////////// delete a key
        QString res=keysList2->currentItem()->text(0)+" ("+keysList2->currentItem()->text(1)+")";
        int result=KMessageBox::warningContinueCancel(this,
                        i18n("<p>Delete <b>SECRET KEY</b> pair <b>%1</b>?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key again.").arg(res),
                        i18n("Warning"),
                        KGuiItem(i18n("Delete"),"editdelete"));
        if (result!=KMessageBox::Continue)
                return;

        KProcess *conprocess=new KProcess();
        KConfig *config = KGlobal::config();
        config->setGroup("General");
        *conprocess<< config->readPathEntry("TerminalApplication","konsole");
        *conprocess<<"-e"<<"gpg"
        <<"--no-secmem-warning"
        <<"--delete-secret-key"<<keysList2->currentItem()->text(6);
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(reloadSecretKeys()));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::reloadSecretKeys()
{
        FILE *fp;
        char line[300];
        keysList2->secretList=QString::null;
        fp = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                QString lineRead=QString::fromUtf8(line);
                if (lineRead.startsWith("sec"))
                        keysList2->secretList+="0x"+lineRead.section(':',4,4).right(8)+",";
        }
        pclose(fp);
        deletekey();
}

void listKeys::confirmdeletekey()
{
        if (keysList2->currentItem()->depth()!=0) {
                if ((keysList2->currentItem()->depth()==1) && (keysList2->currentItem()->text(4)=="-") && (keysList2->currentItem()->text(6).startsWith("0x")))
                        delsignkey();
                return;
        }
        if (keysList2->currentItem()->text(6).isEmpty()) {
                deleteGroup();
                return;
        }
        if (((keysList2->secretList.find(keysList2->currentItem()->text(6))!=-1) || (keysList2->orphanList.find(keysList2->currentItem()->text(6))!=-1)) && (keysList2->selectedItems().count()==1))
                deleteseckey();
        else {
                QStringList keysToDelete;
                QString secList;
                QPtrList<QListViewItem> exportList=keysList2->selectedItems();
                bool secretKeyInside=false;
                for ( uint i = 0; i < exportList.count(); ++i )
                        if ( exportList.at(i) ) {
				if (keysList2->secretList.find(exportList.at(i)->text(6))!=-1) {
                                        secretKeyInside=true;
                                        secList+=exportList.at(i)->text(0)+" ("+exportList.at(i)->text(1)+")<br>";
                                        exportList.at(i)->setSelected(false);
                                } else
                                        keysToDelete+=exportList.at(i)->text(0)+" ("+exportList.at(i)->text(1)+")";
                        }

                if (secretKeyInside) {
                        int result=KMessageBox::warningContinueCancel(this,i18n("<qt>The following are secret key pairs:<br><b>%1</b>They will not be deleted.<br></qt>").arg(secList));
                        if (result!=KMessageBox::Continue)
                                return;
                }
                if (keysToDelete.isEmpty())
                        return;
		int result=KMessageBox::warningContinueCancelList(this,i18n("<qt><b>Delete the following public key?</b></qt>","<qt><b>Delete the following %n public keys?</b></qt>",keysToDelete.count()),keysToDelete,i18n("Warning"),KStdGuiItem::del());
                if (result!=KMessageBox::Continue)
                        return;
                else
                        deletekey();
        }
}

void listKeys::deletekey()
{
        QPtrList<QListViewItem> exportList=keysList2->selectedItems();
        if (exportList.count()==0)
                return;
        KProcess gp;
        gp << "gpg"
        << "--no-tty"
        << "--no-secmem-warning"
        << "--batch"
        << "--yes"
        << "--delete-key";
        for ( uint i = 0; i < exportList.count(); ++i )
                if ( exportList.at(i) )
                        gp<<(exportList.at(i)->text(6)).stripWhiteSpace();
        gp.start(KProcess::Block);

        for ( uint i = 0; i < exportList.count(); ++i )
                if ( exportList.at(i) )
                        keysList2->refreshcurrentkey(exportList.at(i));
        if (keysList2->currentItem()) {
                QListViewItem * myChild = keysList2->currentItem();
                while(!myChild->isVisible()) {
                        myChild = myChild->nextSibling();
                        if (!myChild)
                                break;
                }
                if (!myChild) {
                        QListViewItem * myChild = keysList2->firstChild();
                        while(!myChild->isVisible()) {
                                myChild = myChild->nextSibling();
                                if (!myChild)
                                        break;
                        }
                }
                if (myChild) {
                        myChild->setSelected(true);
                        keysList2->setCurrentItem(myChild);
                }
        }
	else stateChanged("empty_list");
        changeMessage(i18n("%1 Keys, %2 Groups").arg(keysList2->childCount()-keysList2->groupNb).arg(keysList2->groupNb),1);
}


void listKeys::slotPreImportKey()
{
        KDialogBase *dial=new KDialogBase( KDialogBase::Swallow, i18n("Key Import"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "key_import",true);

        SrcSelect *page=new SrcSelect();
        dial->setMainWidget(page);
        page->newFilename->setCaption(i18n("Open File"));
        page->newFilename->setMode(KFile::File);
        page->resize(page->minimumSize());
        dial->resize(dial->minimumSize());

        if (dial->exec()==QDialog::Accepted) {
                if (page->checkFile->isChecked()) {
                        QString impname=page->newFilename->url().stripWhiteSpace();
                        if (!impname.isEmpty()) {
                                changeMessage(i18n("Importing..."),0,true);
                                ////////////////////////// import from file
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKeyURL(KURL::fromPathOrURL( impname ));
                                connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                                connect(importKeyProcess,SIGNAL(refreshOrphaned()),keysList2,SLOT(slotReloadOrphaned()));
                        }
                } else {
                        QString keystr = kapp->clipboard()->text(clipboardMode);
                        if (!keystr.isEmpty()) {
                                changeMessage(i18n("Importing..."),0,true);
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKey(keystr);
                                connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                                connect(importKeyProcess,SIGNAL(refreshOrphaned()),keysList2,SLOT(slotReloadOrphaned()));
                        }
                }
        }
        delete dial;
}


void KeyView::expandGroup(QListViewItem *item)
{

        QStringList keysGroup=KgpgInterface::getGpgGroupSetting(item->text(0),KGpgSettings::gpgConfigPath());
        kdDebug(2100)<<keysGroup<<endl;
        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {
                UpdateViewItem *item2=new UpdateViewItem(item,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null);
                item2->setPixmap(0,pixkeyGroup);
                item2->setExpandable(false);
        }
}

QPixmap KeyView::slotGetPhoto(QString photoId,bool mini)
{
        KTempFile *phototmp=new KTempFile();
        QString popt="cp %i "+phototmp->name();
        KProcIO *p=new KProcIO(QTextCodec::codecForLocale());
        *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<photoId;
        p->start(KProcess::Block);

        QPixmap pixmap;

        pixmap.load(phototmp->name());
        QImage dup=pixmap.convertToImage();
        QPixmap dup2;
        if (!mini)
                dup2.convertFromImage(dup.scale(previewSize+5,previewSize,QImage::ScaleMin));
        else
                dup2.convertFromImage(dup.scale(22,22,QImage::ScaleMin));
        phototmp->unlink();
        delete phototmp;
        return dup2;
}

void KeyView::expandKey(QListViewItem *item)
{

        if (item->childCount()!=0)
                return;   // key has already been expanded
        FILE *fp;
        QString cycle;
        QStringList tst;
        char tmpline[300];
        UpdateViewItem *itemsub=NULL;
        UpdateViewItem *itemuid=NULL;
        UpdateViewItem *itemsig=NULL;
        UpdateViewItem *itemrev=NULL;
        QPixmap keyPhotoId;
        int uidNumber=2;
        bool dropFirstUid=false;

        kdDebug(2100)<<"Expanding Key: "<<item->text(6)<<endl;

        cycle="pub";
        bool noID=false;
        fp = popen(QFile::encodeName(QString("gpg --no-secmem-warning --no-tty --with-colons --list-sigs "+item->text(6))), "r");

        while ( fgets( tmpline, sizeof(tmpline), fp)) {
                QString line = QString::fromUtf8( tmpline );
                tst=QStringList::split(":",line,true);
                if ((tst[0]=="pub") && (tst[9].isEmpty())) /// Primary User Id is separated from public key
                        uidNumber=1;
                if (tst[0]=="uid" || tst[0]=="uat") {
                        if (dropFirstUid) {
                                dropFirstUid=false;
                        } else {
                                gpgKey uidKey=extractKey(line);

                                if (tst[0]=="uat") {
                                        kdDebug(2100)<<"Found photo at uid "<<uidNumber<<endl;
                                        itemuid= new UpdateViewItem(item,i18n("Photo id"),QString::null,QString::null,"-","-","-",QString::number(uidNumber));
                                        if (displayPhoto) {
                                                kgpgphototmp=new KTempFile();
                                                kgpgphototmp->setAutoDelete(true);
                                                QString pgpgOutput="cp %i "+kgpgphototmp->name();
                                                KProcIO *p=new KProcIO(QTextCodec::codecForLocale());
                                                *p<<"gpg"<<"--no-tty"<<"--photo-viewer"<<QFile::encodeName(pgpgOutput);
                                                *p<<"--edit-key"<<item->text(6)<<"uid"<<QString::number(uidNumber)<<"showphoto"<<"quit";
                                                p->start(KProcess::Block);
                                                QPixmap pixmap;
                                                pixmap.load(kgpgphototmp->name());
                                                QImage dup=pixmap.convertToImage();
                                                QPixmap dup2;
                                                dup2.convertFromImage(dup.scale(previewSize+5,previewSize,QImage::ScaleMin));
                                                itemuid->setPixmap(0,dup2);
                                                delete kgpgphototmp;
                                                //itemuid->setPixmap(0,keyPhotoId);
                                        } else
                                                itemuid->setPixmap(0,pixuserphoto);
                                        itemuid->setPixmap(2,uidKey.trustpic);
                                        cycle="uid";
                                } else {
                                        kdDebug(2100)<<"Found uid at "<<uidNumber<<endl;
                                        itemuid= new UpdateViewItem(item,uidKey.gpgkeyname,uidKey.gpgkeymail,QString::null,"-","-","-","-");
                                        itemuid->setPixmap(2,uidKey.trustpic);
                                        if (noID) {
                                                item->setText(0,uidKey.gpgkeyname);
                                                item->setText(1,uidKey.gpgkeymail);
                                                noID=false;
                                        }
                                        itemuid->setPixmap(0,pixuserid);
                                        cycle="uid";
                                }
                        }
                        uidNumber++;
                } else
                        if (tst[0]=="rev") {
                                gpgKey revKey=extractKey(line);
                                if (cycle=="uid" || cycle=="uat")
                                        itemrev= new UpdateViewItem(itemuid,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
                                else if (cycle=="pub") { //////////////public key revoked
                                        itemrev= new UpdateViewItem(item,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
                                        dropFirstUid=true;
                                } else if (cycle=="sub")
                                        itemrev= new UpdateViewItem(itemsub,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
                                itemrev->setPixmap(0,pixRevoke);
                        } else


                                if (tst[0]=="sig") {
                                        gpgKey sigKey=extractKey(line);

                                        if (tst[10].endsWith("l"))
                                                sigKey.gpgkeymail+=i18n(" [local]");

                                        if (cycle=="pub")
                                                itemsig= new UpdateViewItem(item,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="sub")
                                                itemsig= new UpdateViewItem(itemsub,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="uid")
                                                itemsig= new UpdateViewItem(itemuid,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);

                                        itemsig->setPixmap(0,pixsignature);
                                } else
                                        if (tst[0]=="sub") {
                                                gpgKey subKey=extractKey(line);
                                                itemsub= new UpdateViewItem(item,i18n("%1 subkey").arg(subKey.gpgkeyalgo),QString::null,QString::null,subKey.gpgkeyexpiration,subKey.gpgkeysize,subKey.gpgkeycreation,subKey.gpgkeyid);
                                                itemsub->setPixmap(0,pixkeySingle);
                                                itemsub->setPixmap(2,subKey.trustpic);
                                                cycle="sub";

                                        }
        }
        pclose(fp);
}


void listKeys::refreshkey()
{
        keysList2->refreshkeylist();
	listViewSearch->updateSearch(listViewSearch->text());
}

void KeyView::refreshkeylist()
{
        emit statusMessage(i18n("Loading Keys..."),0,true);
        kapp->processEvents();
        ////////   update display of keys in main management window
        kdDebug(2100)<<"Refreshing key list"<<endl;
        QString tst;
        char line[300];
        UpdateViewItem *item=NULL;
        bool noID=false;
        bool emptyList=true;
        QString openKeys;

        // get current position.
        QListViewItem *current = currentItem();
        if(current != NULL) {
                while(current->depth() > 0) {
                        current = current->parent();
                }
                takeItem(current);
        }

        // refill
        clear();
        FILE *fp2,*fp;
        QStringList issec;
        secretList=QString::null;
        orphanList=QString::null;
        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=QString::fromUtf8(line);
                kdDebug(2100) << k_funcinfo << "Read one secret key line: " << lineRead << endl;
                if (lineRead.startsWith("sec"))
                        issec<<lineRead.section(':',4,4).right(8);
        }
        pclose(fp2);

        QString defaultKey = KGpgSettings::defaultKey();
        fp = popen("gpg --no-secmem-warning --no-tty --with-colons --list-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=QString::fromUtf8(line);
                kdDebug(2100) << k_funcinfo << "Read one public key line: " << tst << endl;
                if (tst.startsWith("pub")) {
                        emptyList=false;
                        noID=false;
                        gpgKey pubKey=extractKey(tst);

                        bool isbold=false;
                        bool isexpired=false;
                        if (pubKey.gpgkeyid==defaultKey)
                                isbold=true;
                        if (pubKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        if (pubKey.gpgkeyname.isEmpty())
                                noID=true;

                        item=new UpdateViewItem(this,pubKey.gpgkeyname,pubKey.gpgkeymail,QString::null,pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);

                        item->setPixmap(2,pubKey.trustpic);
                        item->setExpandable(true);


                        QStringList::Iterator ite;
                        ite=issec.find(pubKey.gpgkeyid.right(8));
                        if (ite!=issec.end()) {
                                item->setPixmap(0,pixkeyPair);
                                secretList+=pubKey.gpgkeyid;
                                issec.remove(*ite);
                        } else item->setPixmap(0,pixkeySingle);

                        if (openKeys.find(pubKey.gpgkeyid)!=-1)
                                item->setOpen(true);
                }

        }
        pclose(fp);
        if (!issec.isEmpty())
                insertOrphanedKeys(issec);
        if (emptyList) {
                kdDebug(2100)<<"No key found"<<endl;
                emit statusMessage(i18n("Ready"),0);
                return;
        }
        kdDebug(2100)<<"Checking Groups"<<endl;
        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
        groupNb=groups.count();
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null,false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
                }
        kdDebug(2100)<<"Finished Groups"<<endl;

        QListViewItem *newPos=0L;
        if(current != NULL) {
                // select previous selected
                if (!current->text(6).isEmpty())
                        newPos = findItem(current->text(6), 6);
                else
                        newPos = findItem(current->text(0), 0);
                delete current;
        }

        if (newPos != 0L) {
                setCurrentItem(newPos);
                setSelected(newPos, true);
                ensureItemVisible(newPos);
        } else {
                setCurrentItem(firstChild());
                setSelected(firstChild(),true);
        }

        emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount()-groupNb).arg(groupNb),1);
        emit statusMessage(i18n("Ready"),0);
        kdDebug(2100)<<"Refresh Finished"<<endl;
}

void KeyView::insertOrphan(QString currentID)
{
        FILE *fp;
        char line[300];
        UpdateViewItem *item=NULL;
        bool keyFound=false;
        fp = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                QString lineRead=QString::fromUtf8(line);
                if ((lineRead.startsWith("sec")) && (lineRead.section(':',4,4).right(8))==currentID.right(8)) {
                        gpgKey orphanedKey=extractKey(lineRead);
                        keyFound=true;
                        bool isbold=false;
                        bool isexpired=false;
                        //           if (orphanedKey.gpgkeyid==defaultKey)
                        //               isbold=true;
                        if (orphanedKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        //          if (orphanedKey.gpgkeyname.isEmpty())
                        //              noID=true;

                        item=new UpdateViewItem(this,orphanedKey.gpgkeyname,orphanedKey.gpgkeymail,QString::null,orphanedKey.gpgkeyexpiration,orphanedKey.gpgkeysize,orphanedKey.gpgkeycreation,orphanedKey.gpgkeyid,isbold,isexpired);
                        item->setPixmap(0,pixkeyOrphan);
                }
        }
        pclose(fp);
        if (!keyFound) {
                orphanList.remove(currentID);
                setSelected(currentItem(),true);
                return;
        }
        clearSelection();
        setCurrentItem(item);
        setSelected(item,true);
}

void KeyView::insertOrphanedKeys(QStringList orphans)
{
        FILE *fp;
        char line[300];
        fp = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                QString lineRead=QString::fromUtf8(line);
                if ((lineRead.startsWith("sec")) && (orphans.find(lineRead.section(':',4,4).right(8))!=orphans.end())) {
                        gpgKey orphanedKey=extractKey(lineRead);

                        bool isbold=false;
                        bool isexpired=false;
                        //           if (orphanedKey.gpgkeyid==defaultKey)
                        //               isbold=true;
                        if (orphanedKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        //          if (orphanedKey.gpgkeyname.isEmpty())
                        //              noID=true;
                        orphanList+=orphanedKey.gpgkeyid+",";
                        UpdateViewItem *item=new UpdateViewItem(this,orphanedKey.gpgkeyname,orphanedKey.gpgkeymail,QString::null,orphanedKey.gpgkeyexpiration,orphanedKey.gpgkeysize,orphanedKey.gpgkeycreation,orphanedKey.gpgkeyid,isbold,isexpired);
                        item->setPixmap(0,pixkeyOrphan);
                }
        }
        pclose(fp);
}

void KeyView::refreshgroups()
{
        QListViewItem *item=firstChild();
        while (item) {
                if (item->text(6).isEmpty()) {
                        QListViewItem *item2=item->nextSibling();
                        delete item;
                        item=item2;
                } else
                        item=item->nextSibling();
        }

        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
        groupNb=groups.count();
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null,false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
                }
        emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount()-groupNb).arg(groupNb),1);
        emit statusMessage(i18n("Ready"),0);
}

void KeyView::refreshselfkey()
{
        kdDebug(2100)<<"Refreshing key"<<endl;
        if (currentItem()->depth()==0)
                refreshcurrentkey(currentItem());
        else
                refreshcurrentkey(currentItem()->parent());
}

void KeyView::slotReloadKeys(QStringList keyIDs)
{
        if (keyIDs.isEmpty())
                return;
	if (keyIDs.first()=="ALL")
	{
	refreshkeylist();
	return;
	}
        for ( QStringList::Iterator it = keyIDs.begin(); it != keyIDs.end(); ++it ) {
                refreshcurrentkey(*it);
        }
        kdDebug(2100)<<"Refreshing key:--------"<<(keyIDs.last()).right(8).prepend("0x")<<endl;
        ensureItemVisible(this->findItem((keyIDs.last()).right(8).prepend("0x"),6));
        emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount()-groupNb).arg(groupNb),1);
        emit statusMessage(i18n("Ready"),0);
}

void KeyView::slotReloadOrphaned()
{
        QStringList issec;
        FILE *fp,*fp2;
        char line[300];

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=QString::fromUtf8(line);
                if (lineRead.startsWith("sec"))
                        issec<<"0x"+lineRead.section(':',4,4).right(8);
        }
        pclose(fp2);

        fp = popen("gpg --no-secmem-warning --no-tty --with-colons --list-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                QString lineRead=QString::fromUtf8(line);
                if (lineRead.startsWith("pub"))
                        issec.remove("0x"+lineRead.section(':',4,4).right(8));
        }
        pclose(fp);

        QStringList::Iterator it;

        for ( it = issec.begin(); it != issec.end(); ++it ) {
                if (findItem(*it,6)==0) {
                        insertOrphan(*it);
                        orphanList+=*it+",";
                }
        }
        setSelected(findItem(*it,6),true);
        emit statusMessage(i18n("%1 Keys, %2 Groups").arg(childCount()-groupNb).arg(groupNb),1);
        emit statusMessage(i18n("Ready"),0);
}

void KeyView::refreshcurrentkey(QString currentID)
{
	if (currentID.isNull()) return;
        UpdateViewItem *item=NULL;
        QString issec=QString::null;
        FILE *fp,*fp2;
        char line[300];

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colons --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=QString::fromUtf8(line);
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);

        QString defaultKey = KGpgSettings::defaultKey();

        QString tst;
        bool keyFound=false;
        QString cmd="gpg --no-secmem-warning --no-tty --with-colons --list-keys "+currentID;
        fp = popen(QFile::encodeName(cmd), "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=QString::fromUtf8(line);
                if (tst.startsWith("pub")) {
                        gpgKey pubKey=extractKey(tst);
                        keyFound=true;
                        bool isbold=false;
                        bool isexpired=false;
                        if (pubKey.gpgkeyid==defaultKey)
                                isbold=true;
                        if (pubKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        item=new UpdateViewItem(this,pubKey.gpgkeyname,pubKey.gpgkeymail,QString::null,pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);
                        item->setPixmap(2,pubKey.trustpic);
			item->setVisible(true);
                        item->setExpandable(true);
                        if (issec.find(pubKey.gpgkeyid.right(8),0,FALSE)!=-1) {
                                item->setPixmap(0,pixkeyPair);
                                secretList+=pubKey.gpgkeyid;
                        } else {
                                item->setPixmap(0,pixkeySingle);
                        }
                }
        }
        pclose(fp);

        if (!keyFound) {
                if (orphanList.find(currentID)==-1)
                        orphanList+=currentID+",";
                insertOrphan(currentID);
                return;
        }
        if (orphanList.find(currentID)!=-1)
                orphanList.remove(currentID);

        clearSelection();
        setCurrentItem(item);

}

void KeyView::refreshcurrentkey(QListViewItem *current)
{
        if (!current)
                return;
        bool keyIsOpen=false;
        QString keyUpdate=current->text(6);
        if (keyUpdate.isEmpty())
                return;
        if (current->isOpen())
                keyIsOpen=true;
        delete current;
        refreshcurrentkey(keyUpdate);
        if (currentItem())
                if (currentItem()->text(6)==keyUpdate)
                        currentItem()->setOpen(keyIsOpen);
}


void KeyView::refreshTrust(int color,QColor newColor)
{
if (!newColor.isValid()) return;
QPixmap blankFrame,newtrust;
int trustFinger=0;
blankFrame.load(locate("appdata", "pics/kgpg_blank.png"));
newtrust.load(locate("appdata", "pics/kgpg_fill.png"));
newtrust.fill(newColor);
bitBlt(&newtrust,0,0,&blankFrame,0,0,50,15);
switch (color)
{
case GoodColor:
trustFinger=trustgood.serialNumber();
trustgood=newtrust;
break;
case BadColor:
trustFinger=trustbad.serialNumber();
trustbad=newtrust;
break;
case UnknownColor:
trustFinger=trustunknown.serialNumber();
trustunknown=newtrust;
break;
case RevColor:
trustFinger=trustrevoked.serialNumber();
trustrevoked=newtrust;
break;
}
QListViewItem *item=firstChild();
                while (item) {
			if (item->pixmap(2))
			{
                        if (item->pixmap(2)->serialNumber()==trustFinger) item->setPixmap(2,newtrust);
			}
			item=item->nextSibling();
                }
}

gpgKey KeyView::extractKey(QString keyColon)
{
        QStringList keyString=QStringList::split(":",keyColon,true);
        gpgKey ret;

        ret.gpgkeysize=keyString[2];
        ret.gpgkeycreation=keyString[5];
        if(!ret.gpgkeycreation.isEmpty()) {
                QDate date = QDate::fromString(ret.gpgkeycreation, Qt::ISODate);
                ret.gpgkeycreation=KGlobal::locale()->formatDate(date, true);
        }
        QString tid=keyString[4];
        ret.gpgkeyid=QString("0x"+tid.right(8));
        ret.gpgkeyexpiration=keyString[6];
        if (ret.gpgkeyexpiration.isEmpty())
                ret.gpgkeyexpiration=i18n("Unlimited");
        else {
                QDate date = QDate::fromString(ret.gpgkeyexpiration, Qt::ISODate);
                ret.gpgkeyexpiration=KGlobal::locale()->formatDate(date, true);
        }
        QString fullname=keyString[9];
        if (fullname.find("<")!=-1) {
                ret.gpgkeymail=fullname.section('<',-1,-1);
                ret.gpgkeymail.truncate(ret.gpgkeymail.length()-1);
                ret.gpgkeyname=fullname.section('<',0,0);
                //ret.gpgkeyname=ret.gpgkeyname.section('(',0,0);
        } else {
                ret.gpgkeymail=QString::null;
		ret.gpgkeyname=fullname;
                //ret.gpgkeyname=fullname.section('(',0,0);
        }

        //ret.gpgkeyname=KgpgInterface::checkForUtf8(ret.gpgkeyname); // FIXME lukas

        QString algo=keyString[3];
        if (!algo.isEmpty())
                switch( algo.toInt() ) {
                case  1:
                        algo=i18n("RSA");
                        break;
                case 16:
                case 20:
                        algo=i18n("ElGamal");
                        break;
                case 17:
                        algo=i18n("DSA");
                        break;
                default:
                        algo=QString("#" + algo);
                        break;
                }
        ret.gpgkeyalgo=algo;

        const QString trust=keyString[1];
        switch( trust[0] ) {
        case 'o':
                ret.gpgkeytrust=i18n("Unknown");
                ret.trustpic=trustunknown;
                break;
        case 'i':
                ret.gpgkeytrust=i18n("Invalid");
                ret.trustpic=trustbad;
                break;
        case 'd':
                ret.gpgkeytrust=i18n("Disabled");
                ret.trustpic=trustbad;
                break;
        case 'r':
                ret.gpgkeytrust=i18n("Revoked");
                ret.trustpic=trustrevoked;
                break;
        case 'e':
                ret.gpgkeytrust=i18n("Expired");
                ret.trustpic=trustbad;
                break;
        case 'q':
                ret.gpgkeytrust=i18n("Undefined");
                ret.trustpic=trustunknown;
                break;
        case 'n':
                ret.gpgkeytrust=i18n("None");
                ret.trustpic=trustunknown;
                break;
        case 'm':
                ret.gpgkeytrust=i18n("Marginal");
                ret.trustpic=trustbad;
                break;
        case 'f':
                ret.gpgkeytrust=i18n("Full");
                ret.trustpic=trustgood;
                break;
        case 'u':
                ret.gpgkeytrust=i18n("Ultimate");
                ret.trustpic=trustgood;
                break;
        default:
                ret.gpgkeytrust=i18n("?");
                ret.trustpic=trustunknown;
                break;
        }
        if (keyString[11].find('D')!=-1) {
                ret.gpgkeytrust=i18n("Disabled");
                ret.trustpic=trustbad;
        }

        return ret;
}

#include "listkeys.moc"
