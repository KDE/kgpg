/***************************************************************************
                          listkeys.cpp  -  description
                             -------------------
    begin                : Thu Jul 4 2002
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
#include <qtimer.h>
#include <qpaintdevicemetrics.h>
#include <qtooltip.h>
#include <qheader.h>


#include <kio/netaccess.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kshortcut.h>
#include <kstdaccel.h>
#include <klocale.h>
#include <ktip.h>
#include <krun.h>
#include <kprinter.h>
#include <kurldrag.h>
#include <kwin.h>


#include <kabc/stdaddressbook.h>
#include <kabc/addresseedialog.h>

#include "listkeys.h"
#include "keyexport.h"
#include "sourceselect.h"
#include "adduid.h"
#include "keyservers.h"
#include "kgpginterface.h"
#include "kgpgsettings.h"


//////////////  KListviewItem special

class UpdateViewItem : public KListViewItem
{
public:
        UpdateViewItem(QListView *parent, QString name,QString email, QString tr, QString val, QString size, QString creat, QString id,bool isdefault,bool isexpired);
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
        virtual int compare(  QListViewItem * item, int c, bool ascending ) const;
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


void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        QColorGroup _cg( cg );
        if ((def) && (column<2)) {
                QFont font(p->font());
                font.setBold(true);
                p->setFont(font);
        }
        if ((exp) && (column==3)) {
                _cg.setColor( QColorGroup::Text, Qt::red );
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
        } else if (c==2)   /* sorting by pixmap */
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
        } else {
                rc = item->text(c).lower().compare(text(c).lower());
        }

        // if we aren't comparing by name and we have an equal value,
        // in the column that we are sorting in, do a secondary sort on the name
        if (c > 1 && rc == 0) {
                // trick QListView into also sorting ascending on first column
                if (ascending) {
                        rc = text(0).lower().compare(item->text(0).lower());
                } else {
                        rc = item->text(0).lower().compare(text(0).lower());
                }
        }

        return rc;
}


class SmallViewItem : public KListViewItem
{
public:
        SmallViewItem(QListViewItem *parent=0, QString name=QString::null,QString email=QString::null, QString tr=QString::null, QString val=QString::null, QString size=QString::null, QString creat=QString::null, QString id=QString::null);
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
};

SmallViewItem::SmallViewItem(QListViewItem *parent, QString name,QString email, QString tr, QString val, QString size, QString creat, QString id)
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


void SmallViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        if (column<2) {
                QFont font(p->font());
                //font.setPointSize(font.pointSize()-1);
                font.setItalic(true);
                p->setFont(font);
        }
        KListViewItem::paintCell(p, cg, column, width, alignment);
}





////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////   Secret key selection dialog, used when user wants to sign a key
KgpgSelKey::KgpgSelKey(QWidget *parent, const char *name):KDialogBase( parent, name, true,i18n("Private Key List"),Ok | Cancel)
{
        QString keyname;
        page = new QWidget(this);
        QLabel *labeltxt;
        KIconLoader *loader = KGlobal::iconLoader();

        keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);

        setMinimumSize(300,200);
        keysListpr = new KListView( page );
        keysListpr->setRootIsDecorated(true);
        keysListpr->addColumn( i18n( "Name" ) );
        keysListpr->setShowSortIndicator(true);
        keysListpr->setFullWidth(true);

        labeltxt=new QLabel(i18n("Choose secret key for signing:"),page);
        vbox=new QVBoxLayout(page,3);

        vbox->addWidget(labeltxt);
        vbox->addWidget(keysListpr);

        QString defaultKeyID= KGpgSettings::defaultKey().right(8);

        FILE *fp,*fp2;
        QString tst,tst2;
        char line[130];
        bool selectedok=false;

        fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("sec")) {
			QStringList keyString=QStringList::split(":",tst,true);
                        const QString trust=keyString[1];
                        QString val=keyString[6];
                        QString id=QString("0x"+keyString[4].right(8));
                        if (val.isEmpty())
                                val=i18n("Unlimited");
                        QString tr;
                        switch( trust[0] ) {
                        case 'o':
                                tr= i18n("Unknown");
                                break;
                        case 'i':
                                tr= i18n("Invalid");
                                break;
                        case 'd':
                                tr=i18n("Disabled");
                                break;
                        case 'r':
                                tr=i18n("Revoked");
                                break;
                        case 'e':
                                tr=i18n("Expired");
                                break;
                        case 'q':
                                tr=i18n("Undefined");
                                break;
                        case 'n':
                                tr=i18n("None");
                                break;
                        case 'm':
                                tr=i18n("Marginal");
                                break;
                        case 'f':
                                tr=i18n("Full");
                                break;
                        case 'u':
                                tr=i18n("Ultimate");
                                break;
                        default:
                                tr=i18n("?");
                                break;
                        }
                        tst=keyString[9];

                        fp2 = popen(QFile::encodeName(QString("gpg --no-tty --with-colon --list-key %1").arg(KShellProcess::quote(id))), "r");
                        bool dead=true;
                        while ( fgets( line, sizeof(line), fp2)) {
                                tst2=line;
                                if (tst2.startsWith("pub")) {
                                        const QString trust2=tst2.section(':',1,1);
                                        switch( trust2[0] ) {
                                        case 'f':
                                                dead=false;
                                                break;
                                        case 'u':
                                                dead=false;
                                                break;
                                        default:
                                                break;
                                        }
                                        const QString owntrust=tst2.section(':',8,8);
                                        switch( owntrust[0] ) {
                                        case 'f':
                                                dead=false;
                                                break;
                                        case 'u':
                                                dead=false;
                                                break;
                                        default:
                                                break;
                                        }
                                if (tst2.section(':',11,11).find('D')!=-1) dead=true;
				}
                        }
                        pclose(fp2);
                        if (!tst.isEmpty() && (!dead)) {
                                KListViewItem *item=new KListViewItem(keysListpr,extractKeyName(tst));
                                KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, expiration: %3").arg(id).arg(tr).arg(val));
                                sub->setSelectable(false);
                                item->setPixmap(0,keyPair);
                                if ((!defaultKeyID.isEmpty()) && (id.right(8)==defaultKeyID)) {
                                        keysListpr->setSelected(item,true);
                                        selectedok=true;
                                }
                        }
                }
        }
        pclose(fp);


        QObject::connect(keysListpr,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(slotpreOk()));
        QObject::connect(keysListpr,SIGNAL(clicked(QListViewItem *)),this,SLOT(slotSelect(QListViewItem *)));


        if (!selectedok)
                keysListpr->setSelected(keysListpr->firstChild(),true);

        page->show();
        resize(this->minimumSize());
        setMainWidget(page);
}

QString KgpgSelKey::extractKeyName(QString fullName)
{
        QString kMail;
        if (fullName.find("<")!=-1) {
                kMail=fullName.section('<',-1,-1);
                kMail.truncate(kMail.length()-1);
        }
        QString kName=fullName.section('<',0,0);
        if (kName.find("(")!=-1)
                kName=kName.section('(',0,0);
        kName=KgpgInterface::checkForUtf8(kName);
        return QString(kMail+" ("+kName+")").stripWhiteSpace();
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
        QString userid;
        /////  emit selected key
        if (keysListpr->currentItem()==NULL)
                return(QString::null);
        else {
                userid=keysListpr->currentItem()->firstChild()->text(0);
                userid=userid.section(',',0,0);
                userid=userid.section(':',1,1);
                userid=userid.stripWhiteSpace();
                return(userid);
        }
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

        pixkeyGroup=loader->loadIcon("kgpg_key3",KIcon::Small,20);
        pixkeyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
        pixkeySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
        pixsignature=loader->loadIcon("signature",KIcon::Small,20);
        pixuserid=loader->loadIcon("kgpg_identity",KIcon::Small,20);
        pixuserphoto=loader->loadIcon("kgpg_photo",KIcon::Small,20);
	pixRevoke=loader->loadIcon("stop",KIcon::Small,20);

        trustunknown.load(locate("appdata", "pics/kgpg_unknown.png"));
        trustbad.load(locate("appdata", "pics/kgpg_bad.png"));
        trustmarginal.load(locate("appdata", "pics/kgpg_marginal.png"));
        trustgood.load(locate("appdata", "pics/kgpg_good.png"));

        connect(this,SIGNAL(expanded (QListViewItem *)),this,SLOT(expandKey(QListViewItem *)));
	header()->setMovingEnabled(false);
        setAcceptDrops(true);
        setDragEnabled(true);
}


void  KeyView::droppedfile (KURL url)
{
        if (KMessageBox::questionYesNo(this,i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>").arg(url.path()))!=KMessageBox::Yes)
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
        QString gpgcmd="gpg --no-tty --export --armor "+KShellProcess::quote(keyid.local8Bit());

        QString keytxt;
        fp=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), fp))    /// read output
                keytxt+=line;
        pclose(fp);

        QDragObject *d = new QTextDrag( keytxt, this );
        d->dragCopy();
        // do NOT delete d.
}



///////////////////////////////////////////////////////////////////////////////////////   main window for key management

listKeys::listKeys(QWidget *parent, const char *name) : DCOPObject( "KeyInterface" ), KMainWindow(parent, name,0)
{
	//KWin::setType(Qt::WDestructiveClose);
        keysList2 = new KeyView(this);
        keysList2->photoKeysList=QString::null;
	
	
        setAutoSaveSettings();
        readOptions();

	if (showTipOfDay)
	installEventFilter(this);
        setCaption(i18n("Key Management"));

        KAction *exportPublicKey = new KAction(i18n("E&xport Public Key(s)..."), "kgpg_export", KStdAccel::shortcut(KStdAccel::Copy),this, SLOT(slotexport()),actionCollection(),"key_export");
        KAction *deleteKey = new KAction(i18n("&Delete Key(s)"),"editdelete", Qt::Key_Delete,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
        signKey = new KAction(i18n("&Sign Key(s)..."), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
        KAction *delSignKey = new KAction(i18n("Delete Sign&ature"),"editdelete", 0,this, SLOT(delsignkey()),actionCollection(),"key_delsign");
        KAction *infoKey = new KAction(i18n("&Edit Key"), "kgpg_info", Qt::Key_Return,this, SLOT(listsigns()),actionCollection(),"key_info");
        KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste),this, SLOT(slotPreImportKey()),actionCollection(),"key_import");
        KAction *setDefaultKey = new KAction(i18n("Set as De&fault Key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
        importSignatureKey = new KAction(i18n("Import Key From Keyserver"),"network", 0,this, SLOT(preimportsignkey()),actionCollection(),"key_importsign");
        importAllSignKeys = new KAction(i18n("Import Missing Signatures From Keyserver"),"network", 0,this, SLOT(importallsignkey()),actionCollection(),"key_importallsign");

        (void) new KAction(i18n("&Create Group With Selected Keys"), 0, 0,this, SLOT(createNewGroup()),actionCollection(),"create_group");
        KAction *delGroup= new KAction(i18n("&Delete Group"), 0, 0,this, SLOT(deleteGroup()),actionCollection(),"delete_group");
        KAction *editCurrentGroup= new KAction(i18n("&Edit Group"), 0, 0,this, SLOT(editGroup()),actionCollection(),"edit_group");

        (void) new KAction(i18n("&Create New Contact in Address Book"), "kaddressbook", 0,this, SLOT(addToKAB()),actionCollection(),"add_kab");
//        (void) new KAction(i18n("&Merge Public Keys in Address Book"), "kaddressbook", 0,this, SLOT(allToKAB()),actionCollection(),"all_kabc");
	(void) new KAction(i18n("&Go to Default Key"), "gohome",QKeySequence(CTRL+Qt::Key_Home) ,this, SLOT(slotGotoDefaultKey()),actionCollection(),"go_default_key");

        KStdAction::quit(this, SLOT(annule()), actionCollection());
        KStdAction::find(this, SLOT(findKey()), actionCollection());
        KStdAction::findNext(this, SLOT(findNextKey()), actionCollection());
        (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(),this, SLOT(refreshkey()),actionCollection(),"key_refresh");
        KAction *openPhoto= new KAction(i18n("&Open Photo"), "image", 0,this, SLOT(slotShowPhoto()),actionCollection(),"key_photo");
	KAction *deletePhoto= new KAction(i18n("&Delete Photo"), "delete", 0,this, SLOT(slotDeletePhoto()),actionCollection(),"delete_photo");
	KAction *addPhoto= new KAction(i18n("&Add Photo"), 0, 0,this, SLOT(slotAddPhoto()),actionCollection(),"add_photo");

	KAction *addUid= new KAction(i18n("&Add User Id"), 0, 0,this, SLOT(slotAddUid()),actionCollection(),"add_uid");
	KAction *delUid= new KAction(i18n("&Delete User Id"), 0, 0,this, SLOT(slotDelUid()),actionCollection(),"del_uid");

	KAction *editKey = new KAction(i18n("&Edit Key in Terminal"), "kgpg_term", QKeySequence(ALT+Qt::Key_Return),this, SLOT(slotedit()),actionCollection(),"key_edit");
        KAction *exportSecretKey = new KAction(i18n("Export Secret Key..."), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
        KAction *revokeKey = new KAction(i18n("Revoke Key..."), 0, 0,this, SLOT(revokeWidget()),actionCollection(),"key_revoke");

        KAction *deleteKeyPair = new KAction(i18n("Delete Key Pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
        KAction *generateKey = new KAction(i18n("&Generate Key Pair..."), "kgpg_gen", KStdAccel::shortcut(KStdAccel::New),this, SLOT(slotgenkey()),actionCollection(),"key_gener");

        (void) new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(keyserver()),actionCollection(),"key_server");
        KStdAction::preferences(this, SLOT(slotOptions()), actionCollection(),"kgpg_config");
        (void) new KAction(i18n("Tip of the &Day"), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
        (void) new KAction(i18n("View GnuPG Manual"), "contents", 0,this, SLOT(slotManpage()),actionCollection(),"gpg_man");
        KStdAction::keyBindings( this, SLOT( slotConfigureShortcuts() ),actionCollection(), "key_bind" );

        KStdAction::configureToolbars(this, SLOT(configuretoolbars() ), actionCollection(), "configuretoolbars");
        setStandardToolBarMenuEnabled(true);

	(void) new KToggleAction(i18n("&Show only Secret Keys"), "kgpg_show", 0,this, SLOT(slotToggleSecret()),actionCollection(),"show_secret");
	keysList2->displayOnlySecret=false;
	
	sTrust=new KToggleAction(i18n("Trust"),0, 0,this, SLOT(slotShowTrust()),actionCollection(),"show_trust");
	sSize=new KToggleAction(i18n("Size"),0, 0,this, SLOT(slotShowSize()),actionCollection(),"show_size");
	sCreat=new KToggleAction(i18n("Creation"),0, 0,this, SLOT(slotShowCreat()),actionCollection(),"show_creat");
	sExpi=new KToggleAction(i18n("Expiration"),0, 0,this, SLOT(slotShowExpi()),actionCollection(),"show_expi");
	
	
	photoProps = new KSelectAction(i18n("&Photo ID's"),"kgpg_photo", actionCollection(), "photo_settings");
	connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));

        // Keep the list in kgpg.kcfg in sync with this one!
  	QStringList list;
  	list.append("Disable");
  	list.append("Small");
  	list.append("Medium");
	list.append("Big");
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
        setDefaultKey->plug(popup);
        popup->insertSeparator();
        importAllSignKeys->plug(popup);

        popupsec=new QPopupMenu();
        exportPublicKey->plug(popupsec);
        signKey->plug(popupsec);
        infoKey->plug(popupsec);
        editKey->plug(popupsec);
        setDefaultKey->plug(popupsec);
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

	setCentralWidget(keysList2);
        keysList2->restoreLayout(KGlobal::config(), "KeyView");
	
	QObject::connect(keysList2,SIGNAL(returnPressed(QListViewItem *)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(selectionChanged ()),this,SLOT(checkList()));
        QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                         this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));
	QObject::connect(keysList2,SIGNAL(destroyed()),this,SLOT(annule()));


        ///////////////    get all keys data
        createGUI("listkeys.rc");
	
	sTrust->setChecked(KGpgSettings::showTrust());
	sSize->setChecked(KGpgSettings::showSize());
	sCreat->setChecked(KGpgSettings::showCreat());
	sExpi->setChecked(KGpgSettings::showExpi());

        if (!KGpgSettings::showToolbar())
                toolBar()->hide();
        checkPhotos();
}


listKeys::~listKeys()
{}


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
else keysList2->slotRemoveColumn(2);
}

void listKeys::slotShowExpi()
{
if (sExpi->isChecked())
keysList2->slotAddColumn(3);
else keysList2->slotRemoveColumn(3);
}

void listKeys::slotShowSize()
{
if (sSize->isChecked())
keysList2->slotAddColumn(4);
else keysList2->slotRemoveColumn(4);
}

void listKeys::slotShowCreat()
{
if (sCreat->isChecked())
keysList2->slotAddColumn(5);
else keysList2->slotRemoveColumn(5);
}


bool listKeys::eventFilter( QObject *, QEvent *e )
{
        if ((e->type() == QEvent::Show) && (showTipOfDay))
	{
            	KTipDialog::showTip(this, QString("kgpg/tips"), false);
		showTipOfDay=false;
	}
        return FALSE;
}


void listKeys::slotToggleSecret()
{
if (!keysList2->displayOnlySecret)
{
QListViewItem *item=keysList2->firstChild();
while (item)
{
if (item->pixmap(0)->serialNumber()!=keysList2->pixkeyPair.serialNumber()) item->setVisible(false);
item=item->nextSibling();
}
keysList2->displayOnlySecret=true;
if (!keysList2->currentItem()->isVisible())
{
QListViewItem *item=keysList2->firstChild();
while (!item->isVisible())
item=item->nextSibling();
keysList2->clearSelection();
keysList2->setCurrentItem(item);
keysList2->setSelected(item,true);
}
}
else
{
QListViewItem *item=keysList2->firstChild();
while (item)
{
item->setVisible(true);
item=item->nextSibling();
}
keysList2->ensureItemVisible(keysList2->currentItem());
keysList2->displayOnlySecret=false;
}
}

void listKeys::slotGotoDefaultKey()
{
	QListViewItem *myDefaulKey = keysList2->findItem(KGpgSettings::defaultKey(),6);
	keysList2->clearSelection();
	keysList2->setCurrentItem(myDefaulKey);
	keysList2->setSelected(myDefaulKey,true);
	keysList2->ensureItemVisible(myDefaulKey);
}

void listKeys::slotDelUid()
{
	QListViewItem *item=keysList2->currentItem();
	while (item->depth()>0) item=item->parent();

	KProcess *conprocess=new KProcess();
	KConfig *config = KGlobal::config();
	config->setGroup("General");
	*conprocess<< config->readEntry("TerminalApplication","konsole");
        *conprocess<<"-e"<<"gpg";
        *conprocess<<"--edit-key"<<item->text(6)<<"uid";
	conprocess->start(KProcess::Block);
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
if (imagePath.isEmpty()) return;
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
if (KMessageBox::questionYesNo(this,i18n("<qt>Are you sure you want to delete Photo id <b>%1</b><br>from key <b>%2 &lt;%3&gt;</b> ?</qt>").arg(keysList2->currentItem()->text(6)).arg(keysList2->currentItem()->parent()->text(0)).arg(keysList2->currentItem()->parent()->text(1)),i18n("Warning"),i18n("Delete"))!=KMessageBox::Yes)
return;

KgpgInterface *delPhotoProcess=new KgpgInterface();
                        delPhotoProcess->KgpgDeletePhoto(keysList2->currentItem()->parent()->text(6),keysList2->currentItem()->text(6));
                        connect(delPhotoProcess,SIGNAL(delPhotoFinished()),this,SLOT(slotUpdatePhoto()));
			connect(delPhotoProcess,SIGNAL(delPhotoError(QString)),this,SLOT(slotGpgError(QString)));
}

void listKeys::slotUpdatePhoto()
{
checkPhotos();
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
        while (newdef)
	{
	if ((keysList2->photoKeysList.find(newdef->text(6))!=-1) && (newdef->childCount ()>0))
	{
	while (newdef->firstChild())
	delete newdef->firstChild();
	keysList2->expandKey(newdef);
	}
        newdef = newdef->nextSibling();
	}
}

void listKeys::configuretoolbars()
{
        saveMainWindowSettings(KGlobal::config(), "MainWindow");
        KEditToolbar dlg(actionCollection(), "listkeys.rc");
        connect(&dlg, SIGNAL(newToolbarConfig()), SLOT(saveToolbarConfig()));
        dlg.exec();
}

/**
 * Save new toolbarconfig.
 */
void listKeys::saveToolbarConfig()
{
        createGUI("listkeys.rc");
        applyMainWindowSettings(KGlobal::config(), "MainWindow");
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

        
        //kdDebug()<<"OPts:"<<searchOptions<<endl;
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
        //kdDebug()<<"end loop"<<endl;

        if (foundItem) {
                //kdDebug()<<"Found: "<<searchText<<endl;
                keysList2->clearSelection();
                keysList2->setCurrentItem(item);
                keysList2->setSelected(item,true);
                keysList2->ensureItemVisible(item);
        } else
                KMessageBox::sorry(this,i18n("<qt>Search string '<b>%1</b>' not found.").arg(searchString));
}

void listKeys::findNextKey()
{
        //kdDebug()<<"find next"<<endl;
        if (searchString.isEmpty())
        {
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
        //kdDebug()<<"Next string:"<<searchText<<endl;
        //kdDebug()<<"Search:"<<searchString<<endl;
        //kdDebug()<<"OPts:"<<searchOptions<<endl;
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
                        //kdDebug()<<"Next string:"<<searchText<<endl;
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

        //QString email=extractKeyMail(keysList2->currentItem()).stripWhiteSpace();
        QString email=keysList2->currentItem()->text(1);

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the Address Book. Please check your installation."));
                return;
        }
        KABC::Addressee::List addressees = ab->findByEmail( email );

        KABC::Addressee a;
        bool newEntry=false;

        if ( addressees.isEmpty() ) {
                a.insertEmail(email);
                newEntry=true;
        } else if (addressees.count()==1) {
                a=addressees.first();
        } else {
                a=KABC::AddresseeDialog::getAddressee(this);
                if (a.isEmpty())
                        return;
        }
        KgpgInterface *ks=new KgpgInterface();
        key.setTextData(ks->getKey(keysList2->currentItem()->text(6),true));
        a.insertKey(key);
        ab->insertAddressee(a);
        KABC::StdAddressBook::save();
        if (newEntry)
                KRun::runCommand( "kaddressbook -a " + KProcess::quote(email) );
        else
                KMessageBox::information(this,i18n("<qt>The public key for <b>%1</b> was saved in the corresponding entry in the Address book.</qt>").arg(email));


        //kapp->dcopClient()->send("kaddressbook","AddressBookServiceIface","importVCard(KURL,bool)",data);
        //kapp->dcopClient()->send("kaddressbook","KAddressBookIface","addEmail(QString)","toto@titi.noe");
        //kapp->dcopClient()->send("kgpg","KeyInterface","listsigns()","");
        //KRun::runCommand( "dcop kaddressbook AddressBookServiceIface importVCard("+Vcard+",true)");
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

void listKeys::slotConfigureShortcuts()
{
        KKeyDialog::configure( actionCollection(), this, true );
}

void listKeys::closeEvent ( QCloseEvent * e )
{
        //kapp->ref(); // prevent KMainWindow from closing the app
        //KMainWindow::closeEvent( e );
	e->accept();
//	hide();
//	e->ignore();
}

void listKeys::keyserver()
{

        keyServer *ks=new keyServer(this);
        ks->exec();
        if (ks)
                delete ks;
        refreshkey();
}



void listKeys::checkPhotos()
{
        keysList2->photoKeysList=QString::null;
        char line[300];
        FILE *fp;
        QString tst;
        QString tstID;
        fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-sigs", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub"))
                        tstID=QString("0x"+tst.section(':',4,4).right(8));
                if (tst.startsWith("uat"))
                        keysList2->photoKeysList+=tstID;
        }
        pclose(fp);
}

void listKeys::checkList()
{
        QPtrList<QListViewItem> exportList=keysList2->selectedItems();
        if (exportList.count()>1)
                stateChanged("multi_selected");
        else {
                if (keysList2->currentItem()->text(6).isEmpty())
                        stateChanged("group_selected");
                else
                        stateChanged("single_selected");
        }
}

void listKeys::annule()
{
	/////////  close window
        close();
}


void listKeys::readOptions()
{

	clipboardMode=QClipboard::Clipboard;
        if (KGpgSettings::useMouseSelection() && (kapp->clipboard()->supportsSelection()))
                 clipboardMode=QClipboard::Selection;

        showTipOfDay= KGpgSettings::showTipOfDay();
}


void listKeys::slotOptions()
{
        if (KConfigDialog::showDialog("settings"))
                return;
        kgpgOptions *optionsDialog=new kgpgOptions(this,"settings");
        connect(optionsDialog,SIGNAL(settingsUpdated()),this,SLOT(readAllOptions()));
        optionsDialog->show();
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
if (newdef) slotSetDefaultKey(newdef);
}

void listKeys::slotSetDefaultKey(QListViewItem *newdef)
{
kdDebug()<<"------------------start ------------"<<endl;
if (!newdef) return;
kdDebug()<<newdef->text(6)<<endl;
kdDebug()<<KGpgSettings::defaultKey()<<endl;
if (newdef->text(6)==KGpgSettings::defaultKey()) return;
        if (newdef->pixmap(2)->serialNumber()!=keysList2->trustgood.serialNumber()) {
                KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
                return;
        }

	QListViewItem *olddef = keysList2->findItem(KGpgSettings::defaultKey(),6);

 	KGpgSettings::setDefaultKey(newdef->text(6));
 	KGpgSettings::writeConfig();
        if (olddef) keysList2->refreshcurrentkey(olddef);
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
                                popupout->exec(pos);
                                return;
                        } else
                                signKey->setEnabled(true);
                }

                if (sel->depth()!=0) {
		kdDebug()<<sel->text(0)<<endl;
                        if ((sel->text(4)=="-") && (sel->text(6).startsWith("0x"))) {
                                if ((sel->text(2)=="-") || (sel->text(2)==i18n("Revoked"))) {
                                        if (sel->text(0).find(i18n("User id not found"))==-1)
                                                importSignatureKey->setEnabled(false);
                                        else
                                                importSignatureKey->setEnabled(true);
                                        popupsig->exec(pos);
                                        return;
                                }
                        }
			else if (sel->text(0)==i18n("Photo id")) popupphoto->exec(pos);
			else if (sel->text(6)==("-")) popupuid->exec(pos);
                } else {
                        keysList2->setSelected(sel,TRUE);
                        if (keysList2->currentItem()->text(6).isEmpty())
                                popupgroup->exec(pos);
                        else {
                                if ((keysList2->secretList.find(sel->text(6))!=-1) && (keysList2->selectedItems().count()==1))
                                        popupsec->exec(pos);
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
                                importKeyProcess->importKeyURL(url);
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
        int result=KMessageBox::questionYesNo(this,warn,i18n("Warning"));
        if (result!=KMessageBox::Yes)
                return;

        QString key=keysList2->currentItem()->text(1);

        QString sname=key.section('@',0,0);
        sname=sname.section('.',0,0);
        sname.append(".asc");
        sname.prepend(QDir::homeDirPath()+"/");
        KURL url=KFileDialog::getSaveURL(sname,"*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

        if(!url.isEmpty()) {
                QFile fgpg(url.path());
                if (fgpg.exists())
                        fgpg.remove();

                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--no-tty"<<"--output"<<QFile::encodeName(url.path())<<"--armor"<<"--export-secret-keys"<<keysList2->currentItem()->text(6);
                p->start(KProcess::Block);

                if (fgpg.exists())
                        KMessageBox::information(this,i18n("Your PRIVATE key \"%1\" was successfully exported.\nDO NOT leave it in an insecure place!").arg(url.path()));
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
                QString key=keysList2->currentItem()->text(1);
                sname=key.section('@',0,0);
                sname=sname.section('.',0,0);
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
		if (page->checkServer->isChecked())
		{
		 keyServer *expServer=new keyServer(0,"server_export",false);
		 expServer->page->exportAttributes->setChecked(exportAttr);
		 QString exportKeysList;
		 for ( uint i = 0; i < exportList.count(); ++i )
                                        if ( exportList.at(i) )
                                                exportKeysList.append(" "+exportList.at(i)->text(6).stripWhiteSpace());
		expServer->slotExport(exportKeysList);
		return;
		}
                KProcIO *p=new KProcIO();
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
                        KProcIO *p=new KProcIO();
			*p<<"gpg"<<"--no-tty"<<"--photo-viewer"<<QFile::encodeName(ptr->desktopEntryName()+" %i")<<"--edit-key"<<keysList2->currentItem()->parent()->text(6)<<"uid"<<keysList2->currentItem()->text(6)<<"showphoto";
                        p->start(KProcess::DontCare,true);
}

void listKeys::listsigns()
{
	//kdDebug()<<"Edit -------------------------------"<<endl;
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0) {
                if (keysList2->currentItem()->text(0)==i18n("Photo id")) {
                        //////////////////////////    display photo
		slotShowPhoto();
                }
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
        if (!keysList2->currentItem()->text(6).isEmpty())
                return;

        int result=KMessageBox::questionYesNo(this,i18n("<qt>Are you sure you want to delete group <b>%1</b> ?</qt>").arg(keysList2->currentItem()->text(0)),i18n("Warning"),i18n("Delete"));
        if (result!=KMessageBox::Yes)
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
                } else
                        KMessageBox::sorry(this,i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>").arg(groupName));
        }
}

void listKeys::groupInit(QStringList keysGroup)
{
        kdDebug()<<"preparing group"<<endl;
        QStringList lostKeys;
	bool foundId;

        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {

                QListViewItem *item=gEdit->availableKeys->firstChild();
                foundId=false;
                while (item) {
                        kdDebug()<<"Searching in key: "<<item->text(0)<<endl;
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
        QStringList keysGroup;

	KDialogBase *dialogGroupEdit=new KDialogBase(KDialogBase::Swallow, i18n("Group Properties"), KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,this,0,true);

	gEdit=new groupEdit();
	gEdit->buttonAdd->setPixmap(KGlobal::iconLoader()->loadIcon("forward",KIcon::Small,20));
	gEdit->buttonRemove->setPixmap(KGlobal::iconLoader()->loadIcon("back",KIcon::Small,20));

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
	gEdit->setMinimumSize(gEdit->sizeHint());
        if (dialogGroupEdit->exec()==QDialog::Accepted) groupChange();
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
                QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --fingerprint "+KShellProcess::quote(keysList2->currentItem()->text(6));
                pass=popen(QFile::encodeName(gpgcmd),"r");
                while ( fgets( line, sizeof(line), pass)) {
                        opt=line;
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
                if (KMessageBox::warningContinueCancelList(this,i18n("<qt>You are about to sign the following keys in one pass.<br><b>If you have not carefully checked all fingerprints, the security of your communications may be compromised !</b></qt>"),signKeyList)!=KMessageBox::Continue)
                        return;
        }


        //////////////////  open a secret key selection dialog (KgpgSelKey, see begining of this file)
        KgpgSelKey *opts=new KgpgSelKey(this);

        QLabel *signCheck = new QLabel(i18n("How carefully have you checked that the key(s) really\n"
                                            "belongs to the person(s) you want to communicate with:"),opts->page);
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
	if (signList.count()!=1) terminalSign->setEnabled(false);

        opts->setMinimumHeight(300);

        if (opts->exec()!=QDialog::Accepted)
	{
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
        if (!terminalSign->isChecked()) signLoop();
	else
	{
	KProcess kp;

	KConfig *config = KGlobal::config();
	config->setGroup("General");
	kp<< config->readEntry("TerminalApplication","konsole");
	kp<<"-e"
        <<"gpg"
        <<"--no-secmem-warning"
	<<"-u"
	<<globalkeyID
        <<"--edit-key"
        <<signList.at(0)->text(6);
        if (globalisLocal) kp<<"lsign";
	else kp<<"sign";
        kp.start(KProcess::Block);
	keysList2->refreshcurrentkey(keysList2->currentItem());
	}
}

void listKeys::signLoop()
{
        if (keyCount<globalCount)
	{
	kdDebug()<<"Sign process for key: "<<keyCount<<" on a total of "<<signList.count()<<endl;
                if ( signList.at(keyCount) ) {
                        KgpgInterface *signKeyProcess=new KgpgInterface();
                        signKeyProcess->KgpgSignKey(signList.at(keyCount)->text(6),globalkeyID,globalkeyMail,globalisLocal,globalChecked);
                        connect(signKeyProcess,SIGNAL(signatureFinished(int)),this,SLOT(signatureResult(int)));
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
        QString missingKeysList;
        QListViewItem *current = keysList2->currentItem()->firstChild();
        if (current==NULL)
                return;
        while ( current->nextSibling() ) {
                if (current->text(0).find(i18n("[User id not found]"))!=-1)
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

void listKeys::importsignkey(QString importKeyId)
{
        ///////////////  sign a key
				kServer=new keyServer(0,"server_dialog",false);
				kServer->page->kLEimportid->setText(importKeyId);
				//kServer->Buttonimport->setDefault(true);
				kServer->slotImport();
				//kServer->show();
				connect( kServer->importpop, SIGNAL( destroyed() ) , this, SLOT( importfinished()));
        //connect( kServer , SIGNAL( destroyed() ) , this, SLOT( refreshkey()));
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

        if (KMessageBox::questionYesNo(this,ask)!=KMessageBox::Yes)
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

        KProcess kp;

	KConfig *config = KGlobal::config();
	config->setGroup("General");
	kp<< config->readEntry("TerminalApplication","konsole");
	kp<<"-e"
        <<"gpg"
        <<"--no-secmem-warning"
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
                                int code=KPasswordDialog::getNewPassword(password,i18n("<b>Enter passphrase for %1</b>:<br>Passphrase should include non alphanumeric characters and random sequences").arg(newKeyMail));
                                if (code!=QDialog::Accepted)
                                        return;
                                if (password.length()<5)
                                        KMessageBox::sorry(this,i18n("This passphrase is not secure enough.\nMinimum length= 5 characters"));
                                else
                                        goodpass=true;
                        }

                        //pop = new QDialog( this,0,false,WStyle_Customize | WStyle_NormalBorder);
                        pop = new KPassivePopup();


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
                        QRect qRect(QApplication::desktop()->screenGeometry());
                        int iXpos=qRect.width()/2-pop->width()/2;
                        int iYpos=qRect.height()/2-pop->height()/2;
                        pop->move(iXpos,iYpos);
                        pop->setAutoDelete(false);

                        KProcIO *proc=new KProcIO();
                        message=QString::null;
                        //*proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--passphrase-fd"<<res<<"--gen-key"<<"-a"<<"kgpg.tmp";
                        *proc<<"gpg"<<"--no-tty"<<"--status-fd=2"<<"--no-secmem-warning"<<"--batch"<<"--gen-key";
                        /////////  when process ends, update dialog infos
                        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this, SLOT(genover(KProcess *)));
                        proc->start(KProcess::NotifyOnExit,true);

                        if (ktype=="ElGamal")
                                proc->writeStdin("Key-Type: 20");
                        else if (ktype=="RSA")
                                proc->writeStdin("Key-Type: 1");
                        else
                        {
                                proc->writeStdin("Key-Type: DSA");
                                proc->writeStdin("Subkey-Type: ELG-E");
                                proc->writeStdin(QString("Subkey-Length:%1").arg(ksize));
                        }
                        proc->writeStdin(QString("Passphrase:%1").arg(password));
                        proc->writeStdin(QString("Key-Length:%1").arg(ksize));
                        proc->writeStdin(QString("Name-Real:%1").arg(newKeyName.utf8()));
                        if (!newKeyMail.isEmpty()) proc->writeStdin(QString("Name-Email:%1").arg(newKeyMail));
                        if (!kcomment.isEmpty())
                                proc->writeStdin(QString("Name-Comment:%1").arg(kcomment.utf8()));
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
		kp<< config->readEntry("TerminalApplication","konsole");
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
        KProcIO *conprocess=new KProcIO();
        *conprocess<< "gpg";
        *conprocess<<"--no-secmem-warning"<<"--with-colon"<<"--fingerprint"<<"--list-keys"<<newKeyName;
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
                        //			kdDebug()<<newkeyFinger<<" test:"<<outp.section(':',9,9)<<endl;
                }
        }
}


void listKeys::newKeyDone(KProcess *)
{
        //        refreshkey();
        if (newkeyID.isEmpty()) {
                KMessageBox::detailedSorry(this,i18n("Something unexpected happened during the key pair creation.\nPlease check details for full log output."),message);
                refreshkey();
                return;
        }
        keysList2->refreshcurrentkey(newkeyID);
        KDialogBase *keyCreated=new KDialogBase( this, "key_created", true,i18n("New Key Pair Created"), KDialogBase::Ok);
        newKey *page=new newKey(keyCreated);
        page->TLname->setText("<b>"+newKeyName+"</b>");
        page->TLemail->setText("<b>"+newKeyMail+"</b>");
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
		if (page->CBdefault->isChecked()) slotSetDefaultKey(newdef);
                else
        	{
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
        //kdDebug()<<"Printing..."<<endl;
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
        int result=KMessageBox::questionYesNo(this,
                                              i18n("<p>Delete <b>SECRET KEY</b> pair <b>%1</b> ?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key anymore!").arg(res),
                                              i18n("Warning"),
                                              i18n("Delete"));
        if (result!=KMessageBox::Yes)
                return;

	KProcess *conprocess=new KProcess();
        KConfig *config = KGlobal::config();
	config->setGroup("General");
	*conprocess<< config->readEntry("TerminalApplication","konsole");
        *conprocess<<"-e"<<"gpg"
        <<"--no-secmem-warning"
        <<"--delete-secret-key"<<keysList2->currentItem()->text(6);
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(deletekey()));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::confirmdeletekey()
{
        if (keysList2->currentItem()->depth()!=0)
	{
		if ((keysList2->currentItem()->depth()==1) && (keysList2->currentItem()->text(4)=="-") && (keysList2->currentItem()->text(6).startsWith("0x")))
		delsignkey();
                return;
		}
        if (keysList2->currentItem()->text(6).isEmpty()) {
                deleteGroup();
                return;
        }
        if ((keysList2->secretList.find(keysList2->currentItem()->text(6))!=-1) && (keysList2->selectedItems().count()==1))
                deleteseckey();
        else {
                QStringList keysToDelete;
                QString secretList;
                QPtrList<QListViewItem> exportList=keysList2->selectedItems();
                bool secretKeyInside=false;
                for ( uint i = 0; i < exportList.count(); ++i )
                        if ( exportList.at(i) ) {
                                if (keysList2->secretList.find(exportList.at(i)->text(6))!=-1) {
                                        secretKeyInside=true;
                                        secretList+=exportList.at(i)->text(0)+" ("+exportList.at(i)->text(1)+")<br>";
                                        exportList.at(i)->setSelected(false);
                                } else
                                        keysToDelete+=exportList.at(i)->text(0)+" ("+exportList.at(i)->text(1)+")";
                        }

                if (secretKeyInside) {
                        int result=KMessageBox::warningContinueCancel(this,i18n("<qt>The following are secret key pairs:<br><b>%1</b>They will not be deleted.<br></qt>").arg(secretList));
                        if (result!=KMessageBox::Continue)
                                return;
                }
		if (keysToDelete.isEmpty()) return;
                int result=KMessageBox::questionYesNoList(this,i18n("<qt><b>Delete the following public key(s)  ?</b></qt>"),keysToDelete,i18n("Warning"),i18n("Delete"));
                if (result!=KMessageBox::Yes)
                        return;
                else deletekey();
        }
}

void listKeys::deletekey()
{
QPtrList<QListViewItem> exportList=keysList2->selectedItems();
		        if (exportList.count()==0) return;

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
kdDebug()<<keysList2->currentItem()->text(0)<<endl;
	if (keysList2->currentItem())
		{
		QListViewItem * myChild = keysList2->currentItem();
		while(!myChild->isVisible()) {
            	myChild = myChild->nextSibling();
		if (!myChild) break;
        	}
		if (!myChild) 
		{
		QListViewItem * myChild = keysList2->firstChild();
		while(!myChild->isVisible()) {
            	myChild = myChild->nextSibling();
		if (!myChild) break;
        	}
		}
		if (myChild) {
		myChild->setSelected(true);
		keysList2->setCurrentItem(myChild);
		}
		}
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
                                ////////////////////////// import from file
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKeyURL(impname);
                                connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                        }
                } else {
                        QString keystr = kapp->clipboard()->text(clipboardMode);
                        if (!keystr.isEmpty()) {
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKey(keystr);
                                connect(importKeyProcess,SIGNAL(importfinished(QStringList)),keysList2,SLOT(slotReloadKeys(QStringList)));
                        }
                }
        }
        delete dial;
}

void KeyView::expandGroup(QListViewItem *item)
{
        
        QStringList keysGroup=KgpgInterface::getGpgGroupSetting(item->text(0),KGpgSettings::gpgConfigPath());
        kdDebug()<<keysGroup<<endl;
        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {
                SmallViewItem *item2=new SmallViewItem(item,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null);
                item2->setPixmap(0,pixkeyGroup);
                item2->setExpandable(false);
        }
}

QPixmap KeyView::slotGetPhoto(QString photoId,bool mini)
{
                KTempFile *phototmp=new KTempFile();
                QString popt="cp %i "+phototmp->name();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<photoId;
                p->start(KProcess::Block);

		QPixmap pixmap;

        pixmap.load(phototmp->name());
	QImage dup=pixmap.convertToImage();
	QPixmap dup2;
	if (!mini)
	dup2.convertFromImage(dup.scale(previewSize+5,previewSize,QImage::ScaleMin));
	else dup2.convertFromImage(dup.scale(22,22,QImage::ScaleMin));
	phototmp->unlink();
        return dup2;
}


void KeyView::expandKey(QListViewItem *item)
{
        //kdDebug()<<"Expanding Key"<<endl;
        if (item->childCount()!=0)
                return;   // key has already been expanded

	photoIdList.clear();

	if (photoKeysList.find(item->text(6))!=-1) // contains a photo id
	{
	KgpgInterface *photoProcess=new KgpgInterface();
        photoProcess->KgpgGetPhotoList(item->text(6));
	itemToOpen=item;
	connect(photoProcess,SIGNAL(signalPhotoList(QStringList)),this,SLOT(slotSetPhotoId(QStringList)));
	}
	else expandKey2(item);
}

void KeyView::slotSetPhotoId(QStringList list)
{
photoIdList=list;
expandKey2(itemToOpen);
}

void KeyView::expandKey2(QListViewItem *item)
{
	FILE *fp;
        QString cycle;
	QStringList tst;
        char line[300];
        SmallViewItem *itemsub=NULL;
        SmallViewItem *itemuid=NULL;
        SmallViewItem *itemsig=NULL;
	SmallViewItem *itemrev=NULL;
        QPixmap keyPhotoId;
	int uidNumber=0;
	bool dropFirstUid=false;

	kdDebug()<<"Expanding Key: "<<item->text(6)<<endl;

	cycle="pub";
        bool noID=false;
        fp = popen(QFile::encodeName(QString("gpg --no-secmem-warning --no-tty --with-colon --list-sigs "+item->text(6))), "r");

        while ( fgets( line, sizeof(line), fp)) {
                tst=QStringList::split(":",line,true);
                if (tst[0]=="uid" || tst[0]=="uat") {
			if (dropFirstUid)
			{
			dropFirstUid=false;
                        }
			else
			{
			gpgKey uidKey=extractKey(line);

                        if (tst[0]=="uat") {
				QString photoUid=QString(*photoIdList.begin());
				itemuid= new SmallViewItem(item,i18n("Photo id"),QString::null,QString::null,"-","-","-",photoUid);
				photoIdList.remove(photoIdList.begin());
                                if (displayPhoto)
				{
				kgpgphototmp=new KTempFile();
                		kgpgphototmp->setAutoDelete(true);
                		QString pgpgOutput="cp %i "+kgpgphototmp->name();
                		KProcIO *p=new KProcIO();
                		*p<<"gpg"<<"--no-tty"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(pgpgOutput);
				*p<<"--edit-key"<<item->text(6)<<"uid"<<photoUid<<"showphoto";
				p->start(KProcess::Block);

				QPixmap pixmap;
        			pixmap.load(kgpgphototmp->name());
				QImage dup=pixmap.convertToImage();
				QPixmap dup2;
				dup2.convertFromImage(dup.scale(previewSize+5,previewSize,QImage::ScaleMin));
				itemuid->setPixmap(0,dup2);
				delete kgpgphototmp;
				//itemuid->setPixmap(0,keyPhotoId);
				}
				else itemuid->setPixmap(0,pixuserphoto);
                                itemuid->setPixmap(2,uidKey.trustpic);
				uidNumber++;
                                cycle="uid";
                        } else {
                                itemuid= new SmallViewItem(item,uidKey.gpgkeyname,uidKey.gpgkeymail,QString::null,"-","-","-","-");
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
                } else
                        if (tst[0]=="rev") {
			 gpgKey revKey=extractKey(line);
			 if (cycle=="uid" || cycle=="uat")
                         itemrev= new SmallViewItem(itemuid,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
			 else if (cycle=="pub")
			 { //////////////public key revoked
			 itemrev= new SmallViewItem(item,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
                         dropFirstUid=true;
			 }
			 else if (cycle=="sub")
			 itemrev= new SmallViewItem(itemsub,revKey.gpgkeyname,revKey.gpgkeymail+i18n(" [Revocation signature]"),"-","-","-",revKey.gpgkeycreation,revKey.gpgkeyid);
			 itemrev->setPixmap(0,pixRevoke);
                        } else


                                if (tst[0]=="sig") {
                                        gpgKey sigKey=extractKey(line);

                                        //QString fsigname=extractKeyName(sigKey.gpgkeyname,sigKey.gpgkeymail);
                                        if (tst[10].endsWith("l"))
                                                sigKey.gpgkeymail+=i18n(" [local]");

                                        if (cycle=="pub")
                                                itemsig= new SmallViewItem(item,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="sub")
                                                itemsig= new SmallViewItem(itemsub,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="uid")
                                                itemsig= new SmallViewItem(itemuid,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);

                                        itemsig->setPixmap(0,pixsignature);
                                } else
                                        if (tst[0]=="sub") {
                                                gpgKey subKey=extractKey(line);
                                                itemsub= new SmallViewItem(item,i18n("%1 subkey").arg(subKey.gpgkeyalgo),QString::null,QString::null,subKey.gpgkeyexpiration,subKey.gpgkeysize,subKey.gpgkeycreation,subKey.gpgkeyid);
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
}

void KeyView::refreshkeylist()
{
        ////////   update display of keys in main management window
        FILE *fp;
        QString tst,cycle,revoked;
        char line[300];
        UpdateViewItem *item=NULL;
        bool noID=false;
        bool emptyList=true;
	QString openKeys;

	// get current position.
        //int colWidth = 120; //QMAX(70, columnWidth(0));
        QListViewItem *current = currentItem();
        if(current != NULL) {
                while(current->depth() > 0) {
                        current = current->parent();
                }
                takeItem(current);
        }

        // refill
        clear();
        FILE *fp2;
        QString issec=QString::null;
        secretList=QString::null;
        revoked=QString::null;
        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=line;
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);
        
        QString defaultKey = KGpgSettings::defaultKey();
        fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
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

			//if ((displayPhoto) &&(photoKeysList.find(pubKey.gpgkeyid)!=-1))
			//item->setPixmap(1,slotGetPhoto(pubKey.gpgkeyid));

			if (issec.find(pubKey.gpgkeyid.right(8),0,FALSE)!=-1) {
                                item->setPixmap(0,pixkeyPair);
                                secretList+=pubKey.gpgkeyid;
                        } else {
                                item->setPixmap(0,pixkeySingle);
				if (displayOnlySecret) item->setVisible(false);
                        }

			if (openKeys.find(pubKey.gpgkeyid)!=-1) item->setOpen(true);
                }

        }
        pclose(fp);
        if (emptyList)
                return;
        QStringList groups=KgpgInterface::getGpgGroupNames(KGpgSettings::gpgConfigPath());
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null,false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
			if (displayOnlySecret) item->setVisible(false);
                }

        if(current != NULL) {
                // select previous selected
                QListViewItem *newPos = findItem(current->text(6), 6);
                if (newPos==0)
                        return;
                setCurrentItem(newPos);
                setSelected(newPos, true);
                ensureItemVisible(newPos);
                delete current;
        } else {
                setCurrentItem(firstChild());
                setSelected(firstChild(),true);
        }
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
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),QString::null,QString::null,QString::null,QString::null,QString::null,QString::null,false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
                }
}

void KeyView::refreshselfkey()
{
kdDebug()<<"Refreshing key"<<endl;
if (currentItem()->depth()==0)
refreshcurrentkey(currentItem());
else refreshcurrentkey(currentItem()->parent());
}

void KeyView::slotReloadKeys(QStringList keyIDs)
{
for ( QStringList::Iterator it = keyIDs.begin(); it != keyIDs.end(); ++it ) {
       		refreshcurrentkey(*it);
		}
ensureItemVisible(this->findItem((*keyIDs.begin()).right(8).prepend("0x"),6));
}

void KeyView::refreshcurrentkey(QString currentID)
{
        UpdateViewItem *item=NULL;
        QString issec=QString::null;
        FILE *fp,*fp2;
        char line[300];

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=line;
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);

        QString defaultKey = KGpgSettings::defaultKey();

        QString tst;
        QString cmd="gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8 "+currentID;
        fp = popen(QFile::encodeName(cmd), "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
                        gpgKey pubKey=extractKey(tst);

                        bool isbold=false;
                        bool isexpired=false;
                        if (pubKey.gpgkeyid==defaultKey)
                                isbold=true;
                        if (pubKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        item=new UpdateViewItem(this,pubKey.gpgkeyname,pubKey.gpgkeymail,QString::null,pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);

                        item->setPixmap(2,pubKey.trustpic);

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
clearSelection();
setCurrentItem(item);
setSelected(item,true);
}

void KeyView::refreshcurrentkey(QListViewItem *current)
{
        if (current==NULL)
                return;
	bool keyIsOpen=false;
        QString keyUpdate=current->text(6);
	if (current->isOpen()) keyIsOpen=true;
	delete current;
        refreshcurrentkey(keyUpdate);
	if (currentItem()->text(6)==keyUpdate) currentItem()->setOpen(keyIsOpen);
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
                if (ret.gpgkeyname.find("(")!=-1)
                        ret.gpgkeyname=ret.gpgkeyname.section('(',0,0);
        } else {
                ret.gpgkeymail=QString::null;
                ret.gpgkeyname=fullname.section('(',0,0);
        }

        ret.gpgkeyname=KgpgInterface::checkForUtf8(ret.gpgkeyname);

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
                ret.trustpic=trustbad;
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
                ret.trustpic=trustmarginal;
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
	if (keyString[11].find('D')!=-1)
	{
	ret.gpgkeytrust=i18n("Disabled");
	ret.trustpic=trustbad;
	}

        return ret;
}

#include "listkeys.moc"
