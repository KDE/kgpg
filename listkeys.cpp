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


#include <kabc/stdaddressbook.h>
#include <kabc/addresseedialog.h>

#include "listkeys.h"
#include "keyservers.h"
#include "kgpginterface.h"



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
        SmallViewItem(QListViewItem *parent=0, QString name="",QString email="", QString tr="", QString val="", QString size="", QString creat="", QString id="");
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

        KConfig *config=kapp->config();
        config->setGroup("GPG Settings");
        QString defaultKeyID=KgpgInterface::getGpgSetting("default-key",config->readPathEntry("gpg_config_path"));


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

        FILE *fp,*fp2;
        QString tst,tst2;
        char line[130];
        bool selectedok=false;

        fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("sec")) {
                        const QString trust=tst.section(':',1,1);
                        QString val=tst.section(':',6,6);
                        QString id=QString("0x"+tst.section(':',4,4).right(8));
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
                        tst=tst.section(":",9,9);

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
                                }
                        }
                        pclose(fp2);
                        if (!tst.isEmpty() && (!dead)) {
                                KListViewItem *item=new KListViewItem(keysListpr,extractKeyName(tst));
                                KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, expiration: %3").arg(id).arg(tr).arg(val));
                                sub->setSelectable(false);
                                item->setPixmap(0,keyPair);
                                if ((!defaultKeyID.isEmpty()) && (id.right(8)==defaultKeyID.right(8))) {
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
                return("");
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
                return("");
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

        trustunknown.load(locate("appdata", "pics/kgpg_unknown.png"));
        trustbad.load(locate("appdata", "pics/kgpg_bad.png"));
        trustgood.load(locate("appdata", "pics/kgpg_good.png"));

        connect(this,SIGNAL(expanded (QListViewItem *)),this,SLOT(expandKey(QListViewItem *)));

        setAcceptDrops(true);
        setDragEnabled(true);
}


void  KeyView::droppedfile (KURL url)
{
        if (KMessageBox::questionYesNo(this,i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>").arg(url.path()))!=KMessageBox::Yes)
                return;

        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKeyURL(url);
        connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkeylist()));
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

listKeys::listKeys(QWidget *parent, const char *name, WFlags f) : DCOPObject( "KeyInterface" ), KMainWindow(parent, name, f)
{

        QWidget *page=new QWidget(this);
        keysList2 = new KeyView(page);
        keysList2->photoKeysList="";
        config=kapp->config();
        setAutoSaveSettings();
        readOptions();

        setCaption(i18n("Key Management"));

        KAction *exportPublicKey = new KAction(i18n("E&xport Public Key(s)..."), "kgpg_export", KStdAccel::shortcut(KStdAccel::Copy),this, SLOT(slotexport()),actionCollection(),"key_export");
        KAction *deleteKey = new KAction(i18n("&Delete Key(s)"),"editdelete", Qt::Key_Delete,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
        signKey = new KAction(i18n("&Sign Key(s)..."), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
        KAction *delSignKey = new KAction(i18n("Delete Sign&ature"),"editdelete", 0,this, SLOT(delsignkey()),actionCollection(),"key_delsign");
        KAction *infoKey = new KAction(i18n("&Edit Key"), "kgpg_info", Qt::Key_Return,this, SLOT(listsigns()),actionCollection(),"key_info");
        KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste),this, SLOT(slotPreImportKey()),actionCollection(),"key_import");
        setDefaultKey = new KAction(i18n("Set as De&fault Key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
        importSignatureKey = new KAction(i18n("Import Key From Keyserver"),"network", 0,this, SLOT(preimportsignkey()),actionCollection(),"key_importsign");
        importAllSignKeys = new KAction(i18n("Import Missing Signatures From Keyserver"),"network", 0,this, SLOT(importallsignkey()),actionCollection(),"key_importallsign");

        createGroup= new KAction(i18n("&Create Group With Selected Keys"), 0, 0,this, SLOT(createNewGroup()),actionCollection(),"create_group");
        delGroup= new KAction(i18n("&Delete Group"), 0, 0,this, SLOT(deleteGroup()),actionCollection(),"delete_group");
        editCurrentGroup= new KAction(i18n("&Edit Group"), 0, 0,this, SLOT(editGroup()),actionCollection(),"edit_group");

        addToAddressBook= new KAction(i18n("&Create New Contact In Address Book"), "kaddressbook", 0,this, SLOT(addToKAB()),actionCollection(),"add_kab");
        (void) new KAction(i18n("&Merge Public Keys In Address Book"), "kaddressbook", 0,this, SLOT(allToKAB()),actionCollection(),"all_kabc");

        KStdAction::quit(this, SLOT(annule()), actionCollection());
        KStdAction::find(this, SLOT(findKey()), actionCollection());
        KStdAction::findNext(this, SLOT(findNextKey()), actionCollection());
        (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(),this, SLOT(refreshkey()),actionCollection(),"key_refresh");
        KAction *openPhoto= new KAction(i18n("&Open Photo"), "image", 0,this, SLOT(slotShowPhoto()),actionCollection(),"key_photo");
	KAction *deletePhoto= new KAction(i18n("&Delete Photo"), "delete", 0,this, SLOT(slotDeletePhoto()),actionCollection(),"delete_photo");
	KAction *addPhoto= new KAction(i18n("&Add Photo"), 0, 0,this, SLOT(slotAddPhoto()),actionCollection(),"add_photo");

	editKey = new KAction(i18n("&Edit Key In Konsole"), "kgpg_edit", 0,this, SLOT(slotedit()),actionCollection(),"key_edit");
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

	photoProps = new KSelectAction(i18n("&Photo ID's"),"kgpg_photo", actionCollection(), "photo_settings");
	connect(photoProps, SIGNAL(activated(int)), this, SLOT(slotSetPhotoSize(int)));
  	QStringList list;
  	list.append("Disable");
  	list.append("Small");
  	list.append("Medium");
	list.append("Big");
  	photoProps->setItems(list);
	config->setGroup("General Options");
	int pSize=config->readNumEntry("photo properties",0);
  	photoProps->setCurrentItem (pSize);
	slotSetPhotoSize(pSize);

        QVBoxLayout *vbox=new QVBoxLayout(page,3);

        keysList2->setRootIsDecorated(true);
        //keysList2->addColumn( i18n( "Keys" ),140);
        keysList2->addColumn( i18n( "Name" ),200);
        keysList2->addColumn( i18n( "Email" ),200);
        keysList2->addColumn( i18n( "Trust" ));
        keysList2->addColumn( i18n( "Expiration" ));
        keysList2->addColumn( i18n( "Size" ));
        keysList2->addColumn( i18n( "Creation" ));
        keysList2->addColumn( i18n( "Id" ));
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
        popup->insertSeparator();
        createGroup->plug(popup);
        addToAddressBook->plug(popup);

        popupsec=new QPopupMenu();
        exportPublicKey->plug(popupsec);
        signKey->plug(popupsec);
        infoKey->plug(popupsec);
        editKey->plug(popupsec);
        setDefaultKey->plug(popupsec);
        popupsec->insertSeparator();
	addPhoto->plug(popupsec);
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


/*        keyPhoto=new QLabel(page);
        keyPhoto->setText(i18n("Photo"));
        keyPhoto->setFixedSize(60,60);
        keyPhoto->setScaledContents(true);
        keyPhoto->setFrameStyle( QFrame::Box | QFrame::Raised );
*/
        vbox->addWidget(keysList2);
        //if (showPhoto==true)
        //vbox->addWidget(keyPhoto);
        //vbox->addWidget(statusbar);
        setCentralWidget(page);
        keysList2->restoreLayout(config,"KeyView");

        QObject::connect(keysList2,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(selectionChanged ()),this,SLOT(checkList()));
        QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                         this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));

        //QObject::connect(keysList2,SIGNAL(dropped(QDropEvent * , QListViewItem *)),this,SLOT(slotDroppedFile(QDropEvent * , QListViewItem *)));


        ///////////////    get all keys data
        createGUI("listkeys.rc");
        if (!configshowToolBar)
                toolBar()->hide();
        //togglePhoto->setChecked(showPhoto);
//        if (!showPhoto)
//                keyPhoto->hide();
//        else
                checkPhotos();
}


listKeys::~listKeys()
{}

void listKeys::slotAddPhoto()
{
QString mess="The image must be a JPEG file. Remember that the image is stored within your public key."
			"If you use a very large picture, your key will become very large as well! Keeping the image "
			"close to 240x288 is a good size to use.";

if (KMessageBox::warningContinueCancel(this,i18n(mess))!=KMessageBox::Continue)
return;

QString imagePath=KFileDialog::getOpenFileName (QString::null,"*.jpg",this);
if (imagePath.isEmpty()) return;
KgpgInterface *addPhotoProcess=new KgpgInterface();
                        addPhotoProcess->KgpgAddPhoto(keysList2->currentItem()->text(6),imagePath);
                        connect(addPhotoProcess,SIGNAL(addPhotoFinished()),keysList2,SLOT(refreshselfkey()));
}

void listKeys::slotDeletePhoto()
{
if (KMessageBox::questionYesNo(this,i18n("<qt>Are you sure you want to delete <b>%1</b><br>from key <b>%2 &lt;%3&gt;</b> ?</qt>").arg(keysList2->currentItem()->text(0)).arg(keysList2->currentItem()->parent()->text(0)).arg(keysList2->currentItem()->parent()->text(1)),i18n("Warning"),i18n("Delete"))!=KMessageBox::Yes)
return;

KgpgInterface *delPhotoProcess=new KgpgInterface();
                        delPhotoProcess->KgpgDeletePhoto(keysList2->currentItem()->parent()->text(6),keysList2->currentItem()->text(0).section(' ',-1));
                        connect(delPhotoProcess,SIGNAL(delPhotoFinished()),keysList2,SLOT(refreshselfkey()));
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
        //kdDebug()<<"String:"<<searchText<<"\n";
        //kdDebug()<<"Search:"<<searchString<<"\n";
        //kdDebug()<<"OPts:"<<searchOptions<<"\n";
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
        //kdDebug()<<"end loop"<<"\n";

        if (foundItem) {
                //kdDebug()<<"Found: "<<searchText<<"\n";
                keysList2->clearSelection();
                keysList2->setCurrentItem(item);
                keysList2->setSelected(item,true);
                keysList2->ensureItemVisible(item);
        } else
                KMessageBox::sorry(this,i18n("<qt>Search string '<b>%1</b>' not found.").arg(searchString));
}

void listKeys::findNextKey()
{
        //kdDebug()<<"find next\n";
        if (searchString.isEmpty())
                return;
        bool foundItem=true;
        QListViewItem *item=keysList2->currentItem();
        if (!item)
                return;
        while(item->depth() > 0)
                item = item->parent();
        item=item->nextSibling();
        QString searchText=item->text(0)+" "+item->text(1)+" "+item->text(6);
        //kdDebug()<<"Next string:"<<searchText<<"\n";
        //kdDebug()<<"Search:"<<searchString<<"\n";
        //kdDebug()<<"OPts:"<<searchOptions<<"\n";
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
                        //kdDebug()<<"Next string:"<<searchText<<"\n";
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

void listKeys::allToKAB()
{
        KABC::Key key;
        QString email;
        QStringList keylist;
        KABC::Addressee a;

        KABC::AddressBook *ab = KABC::StdAddressBook::self();
        if ( !ab->load() ) {
                KMessageBox::sorry(this,i18n("Unable to contact the Address Book. Please check your installation."));
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
                KMessageBox::informationList(this,i18n("The following keys were exported to the Address Book:"),keylist);
        else
                KMessageBox::sorry(this,i18n("No entry matching your keys were found in the Address Book."));
}

void listKeys::slotManpage()
{
        kapp->startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void listKeys::slotTip()
{
        KTipDialog::showTip(this, "kgpg/tips", true);
}

void listKeys::slotConfigureShortcuts()
{
        KKeyDialog::configureKeys( actionCollection(), xmlFile(), true, this );
}

void listKeys::closeEvent ( QCloseEvent * e )
{
        kapp->ref(); // prevent KMainWindow from closing the app
        KMainWindow::closeEvent( e );
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
        keysList2->photoKeysList="";
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
//        displayPhoto();
}

/*
void listKeys::displayPhoto()
{
        if ((!showPhoto) || (keysList2->currentItem()==NULL))
                return;
        if ((keysList2->currentItem()->depth()!=0) && (!keysList2->currentItem()->text(0).startsWith(i18n("Photo ")))) {
                keyPhoto->setText(i18n("No\nphoto"));
                return;
        }
	QString CurrentID;
        if (keysList2->currentItem()->depth()==0)
	CurrentID=keysList2->currentItem()->text(6);
	else CurrentID=keysList2->currentItem()->parent()->text(6);
        if (keysList2->photoKeysList.find(CurrentID)!=-1) {
                kgpgtmp=new KTempFile();
                QString popt="cp %i "+kgpgtmp->name();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<CurrentID;
                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessPhoto(KProcess *)));
                //QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(slotinfoimgread(KProcIO *)));
                p->start(KProcess::NotifyOnExit,true);
        } else
                keyPhoto->setText(i18n("No\nphoto"));
}

void listKeys::slotProcessPhoto(KProcess *)
{
        QPixmap pixmap;
        //pixmap.resize(40,40);
        pixmap.load(kgpgtmp->name());
        keyPhoto->setPixmap(pixmap);
        kgpgtmp->unlink();
}
*/

void listKeys::annule()
{
        /////////  cancel & close window
        //exit(0);
        keysList2->saveLayout(config,"KeyView");
        config->setGroup("General Options");
        config->writeEntry("show toolbar",toolBar()->isVisible());
        config->writeEntry("photo properties",photoProps->currentItem());
        config->sync();
        close();
        //reject();
}


void listKeys::readOptions()
{
        config->setGroup("General Options");
        configshowToolBar=config->readBoolEntry("show toolbar",true);
        showPhoto=config->readBoolEntry("show photo",false);
	keysList2->displayPhoto=showPhoto;

        config->setGroup("User Interface");
        //        keysList2->displayMailFirst=config->readBoolEntry("display_mail_first",true);

	if (config->readBoolEntry("selection_clipboard",false)) {
                // support clipboard selection (if possible)
                if (kapp->clipboard()->supportsSelection())
                        kapp->clipboard()->setSelectionMode(true);
        } else
                kapp->clipboard()->setSelectionMode(false);

        config->setGroup("GPG Settings");
        configUrl=config->readPathEntry("gpg_config_path");
        keysList2->configFilePath=configUrl;
        optionsDefaultKey=KgpgInterface::getGpgSetting("default-key",configUrl);
        QString defaultkey=optionsDefaultKey;
        if (!optionsDefaultKey.isEmpty())
                defaultkey.prepend("0x");

        config->setGroup("Encryption");
        config->writeEntry("default key",defaultkey.right(8));
        config->sync();
        keysList2->defKey=defaultkey;
}


void listKeys::slotOptions()
{
        if (KAutoConfigDialog::showDialog("settings"))
                return;
        kgpgOptions *optionsDialog=new kgpgOptions(this,"settings");
        connect(optionsDialog,SIGNAL(updateSettings()),this,SLOT(readAllOptions()));
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
        QListViewItem *newdef = keysList2->firstChild();
        while (newdef->text(6)!=newID)
                if (newdef->nextSibling())
                        newdef = newdef->nextSibling();
                else
                        break;
        slotSetDefaultKey(newdef);
}

void listKeys::slotSetDefaultKey(QListViewItem *newdef)
{

        if (newdef->pixmap(2)->serialNumber()!=keysList2->trustgood.serialNumber()) {
                KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
                return;
        }

        QListViewItem *olddef = keysList2->firstChild();
        while (olddef->text(6)!=keysList2->defKey)
                if (olddef->nextSibling())
                        olddef = olddef->nextSibling();
                else
                        break;
        keysList2->defKey=newdef->text(6);

        config->setGroup("Encryption");
        config->writeEntry("default key",newdef->text(6).right(8));
        config->setGroup("GPG Settings");
        KgpgInterface::setGpgSetting("default-key",newdef->text(6).right(8),config->readPathEntry("gpg_config_path"));
        keysList2->refreshcurrentkey(olddef);
        keysList2->refreshcurrentkey(newdef);

        QListViewItem *updef = keysList2->firstChild();
        while (updef->text(6)!=newdef->text(6))
                if (updef->nextSibling())
                        updef = updef->nextSibling();
                else
                        break;
        keysList2->clearSelection();
        keysList2->setCurrentItem(updef);
        keysList2->setSelected(updef,true);
        keysList2->ensureItemVisible(updef);
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
                        if ((sel->text(4)=="-") && (sel->text(6)!="-")) {
                                if ((sel->text(2)=="-") || (sel->text(2)==i18n("Revoked"))) {
                                        if (sel->text(0).find(i18n("User id not found"))==-1)
                                                importSignatureKey->setEnabled(false);
                                        else
                                                importSignatureKey->setEnabled(true);
                                        popupsig->exec(pos);
                                        return;
                                }
                        }
			else if (sel->text(0).startsWith(i18n("Photo ")))
			popupphoto->exec(pos);
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
        KgpgRevokeWidget *keyRevoke=new KgpgRevokeWidget(this);
        keyRevoke->keyID->setText(keysList2->currentItem()->text(0)+" ("+keysList2->currentItem()->text(1)+") "+i18n("ID: ")+keysList2->currentItem()->text(6));
        keyRevoke->kURLRequester1->setURL(QDir::homeDirPath()+"/"+keysList2->currentItem()->text(1).section('@',0,0)+".revoke");
        keyRevoke->kURLRequester1->setMode(KFile::File);
        if (keyRevoke->exec()!=QDialog::Accepted)
                return;
        if (keyRevoke->cbSave->isChecked()) {
                slotrevoke(keysList2->currentItem()->text(6),keyRevoke->kURLRequester1->url(),keyRevoke->comboBox1->currentItem(),keyRevoke->textDescription->text());
                if (keyRevoke->cbPrint->isChecked())
                        connect(revKeyProcess,SIGNAL(revokeurl(QString)),this,SLOT(doFilePrint(QString)));
        } else {
                if (keyRevoke->cbPrint->isChecked()) {
                        slotrevoke(keysList2->currentItem()->text(6),"",keyRevoke->comboBox1->currentItem(),keyRevoke->textDescription->text());
                        connect(revKeyProcess,SIGNAL(revokecertificate(QString)),this,SLOT(doPrint(QString)));
                }
        }

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


        KURL u;
        QString sname;

        if (exportList.count()==1) {
                QString key=keysList2->currentItem()->text(1);
                sname=key.section('@',0,0);
                sname=sname.section('.',0,0);
        } else
                sname="keyring";
        sname.append(".asc");
        sname.prepend(QDir::homeDirPath()+"/");
        u.setPath(sname);

        popupName *dial=new popupName(i18n("Export Public Key(s) To"),this, "export_key", u,true);
        dial->exportAttributes->setChecked(true);

        if (dial->exec()==QDialog::Accepted) {
                ////////////////////////// export to file
                QString expname;
                bool exportAttr=dial->exportAttributes->isChecked();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--no-tty";
                if (dial->checkFile->isChecked()) {
                        expname=dial->newFilename->text().stripWhiteSpace();
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
                        if (dial->checkClipboard->isChecked())
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
        ///////////////////////// send key by mail
        KProcIO *proc=new KProcIO();
        QString subj="Public key:";
        *proc<<"kmail"<<"--subject"<<subj<<"--body"<<keys;
        proc->start(KProcess::DontCare);
}

void listKeys::slotProcessExportClip(QString keys)
{
        // if (kapp->clipboard()->supportsSelection())
        //   kapp->clipboard()->setSelectionMode(true);
        kapp->clipboard()->setText(keys);
}


void listKeys::showKeyInfo(QString keyID)
{
        KgpgKeyInfo *opts=new KgpgKeyInfo(this,"key_props",keyID);
        opts->show();
        //delete opts;
        /*
                QListViewItem *current = keysList2->firstChild();
                if (current==NULL)
                        return;
                while ( keyID.find(current->text(6).right(8),0,false)==-1) {
                        if (!current->nextSibling())
                                break;
                        else
                                current = current->nextSibling();
                }
                keysList2->setCurrentItem(current);
                keysList2->refreshcurrentkey(keysList2->currentItem());
        */
}


void listKeys::slotShowPhoto()
{
			 KTrader::OfferList offers = KTrader::self()->query("image/jpeg", "Type == 'Application'");
 			KService::Ptr ptr = offers.first();
 			//KMessageBox::sorry(0,ptr->desktopEntryName());
                        KProcIO *p=new KProcIO();
			*p<<"gpg"<<"--no-tty"<<"--photo-viewer"<<QFile::encodeName(ptr->desktopEntryName()+" %i")<<"--edit-key"<<keysList2->currentItem()->parent()->text(6)<<"uid"<<keysList2->currentItem()->text(0).section(' ',-1)<<"showphoto";
                        p->start(KProcess::DontCare,true);
}

void listKeys::listsigns()
{
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0) {
                if (keysList2->currentItem()->text(0).startsWith(i18n("Photo "))) {
                        //////////////////////////    display photo
		slotShowPhoto();
                }
                        return;
        }

        /////////////   open a key info dialog (KgpgKeyInfo, see begining of this file)
        QString key=keysList2->currentItem()->text(6);
        if (!key.isEmpty()) {
                KgpgKeyInfo *opts=new KgpgKeyInfo(this,"key_props",key);
                if (opts->exec()==QDialog::Accepted) keysList2->refreshcurrentkey(keysList2->currentItem());
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
        KgpgInterface::delGpgGroup(keysList2->currentItem()->text(0),keysList2->configFilePath);
        QListViewItem *item=keysList2->currentItem()->nextSibling();
        if (!item)
                item=keysList2->lastChild();
        keysList2->takeItem(keysList2->currentItem());
        keysList2->setCurrentItem(item);
        keysList2->setSelected(item,true);
        config->setGroup("GPG Settings");
        QStringList groups=KgpgInterface::getGpgGroupNames(keysList2->configFilePath);
        if (!groups.isEmpty())
                config->writeEntry("Groups",groups.join(","));
        else
                config->writeEntry("Groups","");
}

void listKeys::groupChange()
{
        QStringList selected;
        QListViewItem *item=gEdit->groupKeys->firstChild();
        while (item) {
                selected+=item->text(2);
                item=item->nextSibling();
        }
        KgpgInterface::setGpgGroupSetting(keysList2->currentItem()->text(0),selected,keysList2->configFilePath);
}

void listKeys::createNewGroup()
{
        QStringList badkeys,keysGroup;
        kdDebug()<<"creating a new group\n";
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
                QString groupName=KLineEditDlg::getText(i18n("Enter new group name:"),0,0,this);
                if (groupName.isEmpty())
                        return;
                if (!keysGroup.isEmpty()) {
                        if (!badkeys.isEmpty())
                                KMessageBox::informationList(this,i18n("Following keys are not valid or not trusted and will not be added to the group:"),badkeys);
                        KgpgInterface::setGpgGroupSetting(groupName,keysGroup,keysList2->configFilePath);
                        config->setGroup("GPG Settings");
                        QStringList groups=KgpgInterface::getGpgGroupNames(keysList2->configFilePath);
                        if (!groups.isEmpty())
                                config->writeEntry("Groups",groups.join(","));
                        else
                                config->writeEntry("Groups","");
                        keysList2->refreshgroups();
                } else
                        KMessageBox::sorry(this,i18n("<qt>No valid or trusted key was selected. The group <b>%1</b> will not be created.</qt>").arg(groupName));
        }
}

void listKeys::groupInit(QStringList keysGroup)
{
        kdDebug()<<"preparing group\n";
        QString groupName;

        QString groupKeyList=keysGroup.join(" ");
        QString searchString;
        QStringList lostKeys;
        bool foundId;

        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {

                QListViewItem *item=gEdit->availableKeys->firstChild();
                foundId=false;
                while (item) {
                        kdDebug()<<"Searching in key: "<<item->text(0)<<"\n";
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
                KMessageBox::informationList(this,i18n("Following keys are in the group but are not valid or not in your keyring:"),lostKeys);
}

void listKeys::editGroup()
{
        QStringList keysGroup;
        gEdit=new groupEdit(this);
        //connect(gEdit->groupKeys,SIGNAL(dropped (QDropEvent *, QListViewItem *)),this,SLOT(GroupAdd(QDropEvent *, QListViewItem *)));
        connect(gEdit->buttonAdd,SIGNAL(clicked()),this,SLOT(groupAdd()));
        connect(gEdit->buttonRemove,SIGNAL(clicked()),this,SLOT(groupRemove()));
        connect(gEdit->buttonOk,SIGNAL(clicked()),this,SLOT(groupChange()));
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
        keysGroup=KgpgInterface::getGpgGroupSetting(keysList2->currentItem()->text(0),keysList2->configFilePath);
        groupInit(keysGroup);
        gEdit->exec();
        delete gEdit;
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
        signTrust->insertItem(i18n("I will not answer"));
        signTrust->insertItem(i18n("I have not checked at all"));
        signTrust->insertItem(i18n("I have done casual checking"));
        signTrust->insertItem(i18n("I have done very careful checking"));
        opts->vbox->addWidget(signTrust);

        QCheckBox *localSign = new QCheckBox(i18n("Local signature (cannot be exported)"),opts->page);
        opts->vbox->addWidget(localSign);
        opts->setMinimumHeight(300);

        if (opts->exec()==QDialog::Accepted) {
                globalkeyID=QString(opts->getkeyID());
                globalkeyMail=QString(opts->getkeyMail());
                globalisLocal=localSign->isChecked();
                globalChecked=signTrust->currentItem();
        } else {
                delete opts;
                return;
        }
        delete opts;


        //for ( uint i = 0; i < signList.count(); ++i )

        globalCount=0;
        signLoop();
}

void listKeys::signLoop()
{
        if (globalCount<=signList.count()) {

                if ( signList.at(globalCount) ) {
                        KgpgInterface *signKeyProcess=new KgpgInterface();
                        signKeyProcess->KgpgSignKey(signList.at(globalCount)->text(6),globalkeyID,globalkeyMail,globalisLocal,globalChecked);
                        connect(signKeyProcess,SIGNAL(signatureFinished(int)),this,SLOT(signatureResult(int)));
                        while (signList.at(globalCount)->firstChild()!=0)
                                delete signList.at(globalCount)->firstChild();
                        signList.at(globalCount)->setOpen(false);
                }
                globalCount++;
        }
}

void listKeys::signatureResult(int success)
{
        if (success==3)
                keysList2->refreshcurrentkey(signList.at(globalCount-1));
        else if (success==2)
                KMessageBox::sorry(this,i18n("<qt>Bad passphrase, key <b>%1</b> not signed.</qt>").arg(signList.at(globalCount-1)->text(0)+" ("+signList.at(globalCount-1)->text(1)+")"));
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
        kServer=new keyServer(0,"server_dialog",false,WDestructiveClose);
        kServer->kLEimportid->setText(importKeyId);
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
                top->setOpen(false);
                keysList2->refreshcurrentkey(top);
                top->setOpen(true);

                //		delete keysList2->currentItem();
                //refreshkey();
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
        kp<<"konsole"
        <<"-e"
        <<"gpg"
        <<"--no-secmem-warning"
        <<"--edit-key"
        <<keysList2->currentItem()->text(6)
        <<"help";
        kp.start(KProcess::Block);
        refreshkey();
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

                        QVBox *passiveBox=pop->standardView(i18n("Generating new key pair."),"",KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop),wid);


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
                        message="";
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
                        proc->writeStdin(QString("Name-Email:%1").arg(newKeyMail));
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
                        FILE *pass;
                        int status;
                        pid_t pid;
                        QString tst;

                        char line[130];

                        //////////   fork process
                        pid = fork ();
                        if (pid == 0)  //////////  child process =console
                        {
                                pass=popen("konsole -e gpg --gen-key","r");
                                while ( fgets( line, sizeof(line), pass))
                                        tst+=line;
                                pclose(pass);
                        } else if (waitpid (pid, &status, 0) != pid)  ////// parent process wait for end of child
                                status = -1;

                }
                refreshkey();

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
        newkeyID="";
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
                        newkeyID=outp.section(':',4,4).right(8);
                        //			kdDebug()<<newkeyID<<"\n";
                }
                if (outp.startsWith("fpr")) {
                        if (newkeyFinger.lower()==outp.section(':',9,9).lower())
                                continueSearch=false;
                        //			kdDebug()<<newkeyFinger<<" test:"<<outp.section(':',9,9)<<"\n";
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
        page->TLid->setText("<b>0x"+newkeyID+"</b>");
        page->LEfinger->setText(newkeyFinger);
        page->CBdefault->setChecked(true);
        page->show();
        //page->resize(page->maximumSize());
        keyCreated->setMainWidget(page);
        delete pop;
        keyCreated->exec();

        QListViewItem *newdef = keysList2->firstChild();
        while (newdef->text(6)!="0x"+newkeyID)
                if (newdef->nextSibling())
                        newdef = newdef->nextSibling();
                else
                        break;
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
                slotrevoke(newkeyID,"",0,i18n("backup copy"));
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
        //kdDebug()<<"Printing...\n";
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
        *conprocess<< "konsole"<<"-e"<<"gpg"
        <<"--no-secmem-warning"
        <<"--delete-secret-key"<<keysList2->currentItem()->text(6);
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(deletekey()));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::confirmdeletekey()
{
        if (keysList2->currentItem()->depth()!=0)
                return;
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
                int result=KMessageBox::questionYesNoList(this,i18n("<qt><b>Delete the following public key(s)  ?</b></qt>"),keysToDelete,i18n("Warning"),i18n("Delete"));
                if (result!=KMessageBox::Yes)
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
        if (keysList2->currentItem())
                keysList2->currentItem()->setSelected(true);
}


void listKeys::slotPreImportKey()
{
        popupImport *dial=new popupImport(i18n("Import Key From"),this, "import_key");

        if (dial->exec()==QDialog::Accepted) {
                bool importSecret = dial->importSecretKeys->isChecked();

                if (dial->checkFile->isChecked()) {
                        QString impname=dial->newFilename->text().stripWhiteSpace();
                        if (!impname.isEmpty()) {
                                ////////////////////////// import from file
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKeyURL(impname, importSecret);
                                connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkey()));
                        }
                } else {
                        QString keystr = kapp->clipboard()->text();
                        if (!keystr.isEmpty()) {
                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                importKeyProcess->importKey(keystr, importSecret);
                                connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkey()));
                        }
                }
        }
        delete dial;
}
/*
void listKeys::slotPreImportKey()
{
    KURL url=KFileDialog::getOpenURL(QString::null,i18n("*.asc|*.asc Files"), this,i18n("Select Key File to Import"));
    if (url.isEmpty())
        return;

    KgpgInterface *importKeyProcess=new KgpgInterface();
    importKeyProcess->importKey(url);
    connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkey()));
}
*/

void KeyView::expandGroup(QListViewItem *item)
{
        kdDebug()<<"Expanding group\n";
        QStringList keysGroup=KgpgInterface::getGpgGroupSetting(item->text(0),configFilePath);
        kdDebug()<<keysGroup<<"\n";
        for ( QStringList::Iterator it = keysGroup.begin(); it != keysGroup.end(); ++it ) {
                SmallViewItem *item2=new SmallViewItem(item,QString(*it),"","","","","","");
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
        //kdDebug()<<"Expanding Key\n";
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
        QString tst,cycle,revoked;
        char line[300];
        SmallViewItem *itemsub=NULL;
        SmallViewItem *itemuid=NULL;
        SmallViewItem *itemsig=NULL;
        QPixmap keyPhotoId;
	int uidNumber=0;

/*	if (photoKeysList.find(item->text(6))!=-1) // contains a photo id
	{
		keyPhotoId=slotGetPhoto(item->text(6));
	}*/

	kdDebug()<<"Expanding Key: "<<item->text(6)<<"\n";

	cycle="pub";
        bool noID=false;
        fp = popen(QFile::encodeName(QString("gpg --no-secmem-warning --no-tty --with-colon --list-sigs "+item->text(6))), "r");

        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("uid") || tst.startsWith("uat")) {
                        gpgKey uidKey=extractKey(tst);

                        if (tst.startsWith("uat")) {
				QString photoUid=QString(*photoIdList.begin());
				itemuid= new SmallViewItem(item,i18n("Photo id: %1").arg(photoUid),"","","-","-","-","-");
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

				//itemuid->setPixmap(0,keyPhotoId);
				}
				else itemuid->setPixmap(0,pixuserphoto);
                                itemuid->setPixmap(2,uidKey.trustpic);
				uidNumber++;
                                cycle="uid";
                        } else {
                                itemuid= new SmallViewItem(item,uidKey.gpgkeyname,uidKey.gpgkeymail,"","-","-","-","-");
                                itemuid->setPixmap(2,uidKey.trustpic);
                                if (noID) {
                                        item->setText(0,uidKey.gpgkeyname);
                                        item->setText(1,uidKey.gpgkeymail);
                                        noID=false;
                                }
                                itemuid->setPixmap(0,pixuserid);
                                cycle="uid";
                        }
                } else


                        if (tst.startsWith("rev")) {
                                revoked+=QString("0x"+tst.section(':',4,4).right(8)+" ");
                        } else


                                if (tst.startsWith("sig")) {
                                        gpgKey sigKey=extractKey(tst);

                                        //QString fsigname=extractKeyName(sigKey.gpgkeyname,sigKey.gpgkeymail);
                                        if (tst.section(':',10,10).endsWith("l"))
                                                sigKey.gpgkeymail+=i18n(" [local]");

                                        if (cycle=="pub")
                                                itemsig= new SmallViewItem(item,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="sub")
                                                itemsig= new SmallViewItem(itemsub,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="uid")
                                                itemsig= new SmallViewItem(itemuid,sigKey.gpgkeyname,sigKey.gpgkeymail,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);

                                        itemsig->setPixmap(0,pixsignature);
                                } else

                                        if (tst.startsWith("sub")) {
                                                gpgKey subKey=extractKey(tst);
                                                tst=i18n("%1 subkey").arg(subKey.gpgkeyalgo);
                                                itemsub= new SmallViewItem(item,tst,"","",subKey.gpgkeyexpiration,subKey.gpgkeysize,subKey.gpgkeycreation,subKey.gpgkeyid);
                                                itemsub->setPixmap(0,pixkeySingle);
                                                itemsub->setPixmap(2,subKey.trustpic);
                                                cycle="sub";

                                        }
        }
        pclose(fp);

        while (!revoked.isEmpty())   ///////////////   there are revoked sigs in previous key
        {
                bool found=false;
                revoked=revoked.stripWhiteSpace();
                QString currentRevoke=revoked.section(' ',0,0);
                revoked.remove(0,currentRevoke.length());
                revoked=revoked.stripWhiteSpace();

                QListViewItem *current = item->firstChild();
                if (current)
                        if (currentRevoke.find(current->text(6))!=-1)
                        {
                                current->setText(2,i18n("Revoked"));
                                found=true;
                        }

                QListViewItem *subcurrent = current->firstChild();
                if (subcurrent)
                {
                        if (currentRevoke.find(subcurrent->text(6))!=-1) {
                                subcurrent->setText(2,i18n("Revoked"));
                                found=true;
                        }
                        while (subcurrent->nextSibling()) {
                                subcurrent = subcurrent->nextSibling();
                                if (currentRevoke.find(subcurrent->text(6))!=-1) {
                                        subcurrent->setText(2,i18n("Revoked"));
                                        found=true;
                                }
                        }
                }

                while ( current->nextSibling() )
                {
                        current = current->nextSibling();
                        if (currentRevoke.find(current->text(6))!=-1) {
                                current->setText(2,i18n("Revoked"));
                                found=true;
                        }

                        QListViewItem *subcurrent = current->firstChild();
                        if (subcurrent) {
                                if (currentRevoke.find(subcurrent->text(6))!=-1) {
                                        subcurrent->setText(2,i18n("Revoked"));
                                        found=true;
                                }
                                while (subcurrent->nextSibling()) {
                                        subcurrent = subcurrent->nextSibling();
                                        if (currentRevoke.find(subcurrent->text(6))!=-1) {
                                                subcurrent->setText(2,i18n("Revoked"));
                                                found=true;
                                        }
                                }
                        }
                }
                if (!found)
                        (void) new SmallViewItem(item,i18n("Revocation Certificate"),"+","+","+","+","+",currentRevoke);
        }
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
        QString issec="";
        secretList="";
        revoked="";
        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=line;
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);

        fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
                        emptyList=false;
                        noID=false;
                        gpgKey pubKey=extractKey(tst);

                        bool isbold=false;
                        bool isexpired=false;
                        if (pubKey.gpgkeyid==defKey)
                                isbold=true;
                        if (pubKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        if (pubKey.gpgkeyname.isEmpty())
                                noID=true;

                        item=new UpdateViewItem(this,pubKey.gpgkeyname,pubKey.gpgkeymail,"",pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);

                        item->setPixmap(2,pubKey.trustpic);

                        item->setExpandable(true);

			//if ((displayPhoto) &&(photoKeysList.find(pubKey.gpgkeyid)!=-1))
			//item->setPixmap(1,slotGetPhoto(pubKey.gpgkeyid));

			if (issec.find(pubKey.gpgkeyid.right(8),0,FALSE)!=-1) {
                                item->setPixmap(0,pixkeyPair);
                                secretList+=pubKey.gpgkeyid;
                        } else {
                                item->setPixmap(0,pixkeySingle);
                        }

                }

        }
        pclose(fp);
        if (emptyList)
                return;
        QStringList groups=KgpgInterface::getGpgGroupNames(configFilePath);
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),"","","","","","",false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
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
                        takeItem(item);
                        item=item2;
                } else
                        item=item->nextSibling();
        }
        QStringList groups=KgpgInterface::getGpgGroupNames(configFilePath);
        for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
                if (!QString(*it).isEmpty()) {
                        item=new UpdateViewItem(this,QString(*it),"","","","","","",false,false);
                        item->setPixmap(0,pixkeyGroup);
                        item->setExpandable(false);
                }
}

void KeyView::refreshselfkey()
{
if (currentItem()->depth()==0)
refreshcurrentkey(currentItem());
else refreshcurrentkey(currentItem()->parent());
}

void KeyView::refreshcurrentkey(QString currentID)
{
        UpdateViewItem *item=NULL;
        QString issec="";
        FILE *fp,*fp2;
        char line[300];

        fp2 = popen("gpg --no-secmem-warning --no-tty --with-colon --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2)) {
                QString lineRead=line;
                if (lineRead.startsWith("sec"))
                        issec+=lineRead.section(':',4,4);
        }
        pclose(fp2);


        QString tst;
        QString cmd="gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8 "+currentID;
        fp = popen(QFile::encodeName(cmd), "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
                        gpgKey pubKey=extractKey(tst);

                        bool isbold=false;
                        bool isexpired=false;
                        if (pubKey.gpgkeyid==defKey)
                                isbold=true;
                        if (pubKey.gpgkeytrust==i18n("Expired"))
                                isexpired=true;
                        item=new UpdateViewItem(this,pubKey.gpgkeyname,pubKey.gpgkeymail,"",pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);

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
	if (current->isOpen ()) keyIsOpen=true;
        takeItem(current);
        refreshcurrentkey(keyUpdate);
	currentItem()->setOpen(keyIsOpen);
}


gpgKey KeyView::extractKey(QString keyColon)
{
        gpgKey ret;

        ret.gpgkeysize=keyColon.section(':',2,2);
        ret.gpgkeycreation=keyColon.section(':',5,5);
        if(!ret.gpgkeycreation.isEmpty()) {
                QDate date = QDate::fromString(ret.gpgkeycreation, Qt::ISODate);
                ret.gpgkeycreation=KGlobal::locale()->formatDate(date, true);
        }
        QString tid=keyColon.section(':',4,4);
        ret.gpgkeyid=QString("0x"+tid.right(8));
        ret.gpgkeyexpiration=keyColon.section(':',6,6);
        if (ret.gpgkeyexpiration=="")
                ret.gpgkeyexpiration=i18n("Unlimited");
        else {
                QDate date = QDate::fromString(ret.gpgkeyexpiration, Qt::ISODate);
                ret.gpgkeyexpiration=KGlobal::locale()->formatDate(date, true);
        }
        QString fullname=keyColon.section(':',9,9);
        if (fullname.find("<")!=-1) {
                ret.gpgkeymail=fullname.section('<',-1,-1);
                ret.gpgkeymail.truncate(ret.gpgkeymail.length()-1);
                ret.gpgkeyname=fullname.section('<',0,0);
                if (ret.gpgkeyname.find("(")!=-1)
                        ret.gpgkeyname=ret.gpgkeyname.section('(',0,0);
        } else {
                ret.gpgkeymail="";
                ret.gpgkeyname=fullname.section('(',0,0);
        }

        ret.gpgkeyname=KgpgInterface::checkForUtf8(ret.gpgkeyname);

        QString algo=keyColon.section(':',3,3);
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

        const QString trust=keyColon.section(':',1,1);
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
                ret.trustpic=trustunknown;
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

        return ret;
}
#include "listkeys.moc"
