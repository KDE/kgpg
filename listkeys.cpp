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


#include <kio/netaccess.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kshortcut.h>
#include <kstdaccel.h>
#include <klocale.h>
#include <ktip.h>
#include <kurldrag.h>

#include "listkeys.h"
#include "keyservers.h"
#include "kgpginterface.h"



//////////////  KListviewItem special

class UpdateViewItem : public KListViewItem
{
public:
        UpdateViewItem(QListView *parent, QString tst, QString tr, QString val, QString size, QString creat, QString id,bool isdefault,bool isexpired);
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
	virtual QString key(int c,bool ) const;
        bool def,exp;
};

UpdateViewItem::UpdateViewItem(QListView *parent, QString tst, QString tr, QString val, QString size, QString creat, QString id,bool isdefault,bool isexpired)
                : KListViewItem(parent)
{
        def=isdefault;
        exp=isexpired;
        setText(0,tst);
        setText(1,tr);
        setText(2,val);
        setText(3,size);
        setText(4,creat);
        setText(5,id);
}


void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        QColorGroup _cg( cg );
        if ((def) && (column==0)) {
                QFont font(p->font());
                font.setBold(true);
                p->setFont(font);
        }
        if ((exp) && (column==2)) {
                _cg.setColor( QColorGroup::Text, Qt::red );
        }
        KListViewItem::paintCell(p,_cg, column, width, alignment);
}

QString UpdateViewItem :: key(int c,bool ) const{
  QString s;
  if ((c==2) || (c==4))
  {
  QDate d = KGlobal::locale()->readDate(text(c));
  if (d.isValid())
  s.sprintf("%08d",d.toString("yyyyMMdd").toInt());
  else s.sprintf("%08d",50000000);  // unlimited expiration dates are handeled as year 5000, so that they are correctly sorted
  }
  if (c==3)
    /* sorting by int */
    s.sprintf("%08d",text(c).toInt());
   if (c==1)
    /* sorting by pixmap */
    s.sprintf("%08d",pixmap(c)->serialNumber());
  else if ((c==0) || (c==5))
    /* sorting alphanumeric */
    s.sprintf("%s",text(c).ascii());

    return s;
}


class SmallViewItem : public KListViewItem
{
public:
        SmallViewItem(QListViewItem *parent=0, QString tst="", QString tr="", QString val="", QString size="", QString creat="", QString id="");
        virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
};

SmallViewItem::SmallViewItem(QListViewItem *parent, QString tst, QString tr, QString val, QString size, QString creat, QString id)
                : KListViewItem(parent)
{
        setText(0,tst);
        setText(1,tr);
        setText(2,val);
        setText(3,size);
        setText(4,creat);
        setText(5,id);
}


void SmallViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
        if (column==0) {
                QFont font(p->font());
                //font.setPointSize(font.pointSize()-1);
                font.setItalic(true);
                p->setFont(font);
        }
        KListViewItem::paintCell(p, cg, column, width, alignment);
}


////////  window for the key info dialog
KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name,QString sigkey,QColor pix,bool editable):KeyProperties( parent, name)
{

        QString message,fingervalue;
        FILE *pass;
        char line[200]="";
        QString opt,tid;
        bool isphoto=false;
        if (editable) {
                kDateWidget->setEnabled(true);
                cBExpiration->setEnabled(true);
                buttonPass->setEnabled(true);
        }
        QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-key "+KShellProcess::quote(sigkey.local8Bit());

        pass=popen(QFile::encodeName(gpgcmd),"r");
        while ( fgets( line, sizeof(line), pass)) {
                opt=line;
                if (opt.startsWith("uat"))
                        isphoto=true;
                if (opt.startsWith("pub")) {
                        QString algo=opt.section(':',3,3);

                        switch( algo.toInt() ) {
                        case  1:
                                algo="RSA";
                                break;
                        case 16:
                        case 20:
                                algo="ElGamal";
                                break;
                        case 17:
                                algo="DSA";
                                break;
                        default:
                                algo=QString("#" + algo);
                                break;
                        }

                        const QString trust=opt.section(':',1,1);
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
                                tr="?";
                                break;
                        }

                        tid=opt.section(':',4,4);
                        displayedKeyID=QString("0x"+tid.right(8));

                        QString fullname=opt.section(':',9,9);
                        if (opt.section(':',6,6)=="") {
                                isUnlimited=true;
                                cBExpiration->setChecked(true);
                                expirationDate=QDate::currentDate();
                                kDateWidget->setDate(expirationDate);
                        } else {
                                isUnlimited=false;
                                expirationDate= QDate::fromString(opt.section(':',6,6), Qt::ISODate);
                                kDateWidget->setDate(expirationDate);
                        }

                        QDate date = QDate::fromString(opt.section(':',5,5), Qt::ISODate);
                        tLCreation->setText(KGlobal::locale()->formatDate(date));

                        tLLength->setText(opt.section(':',2,2));
                        kLTrust->setText(tr);
                        kLTrust->setPaletteBackgroundColor(pix);


                        const QString otrust=opt.section(':',8,8);
                        switch( otrust[0] ) {
                        case 'f':
                                ownerTrust=i18n("Fully");
                                break;
                        case 'u':
                                ownerTrust=i18n("Ultimately");
                                break;
                        case 'm':
                                ownerTrust=i18n("Marginally");
                                break;
                        case 'n':
                                ownerTrust=i18n("Do NOT trust");
                                break;
                        default:
                                ownerTrust=i18n("Don't know");
                                break;
                        }
                        kCOwnerTrust->setCurrentItem(ownerTrust);
                        tLID->setText(tid);
                        if (fullname.find("<")!=-1) {
                                QString kmail=fullname.section('<',-1,-1);
                                kmail.truncate(kmail.length()-1);
                                tLMail->setText(i18n("<qt><b>%1</b></qt>").arg(kmail));
                        } else
                                tLMail->setText(i18n("none"));

                        QString kname=fullname.section('<',0,0);
                        if (fullname.find("(")!=-1) {
                                kname=kname.section('(',0,0);
                                QString comment=fullname.section('(',1,1);
                                comment=comment.section(')',0,0);
                                tLComment->setText(KgpgInterface::checkForUtf8(comment));
                        } else
                                tLComment->setText(i18n("none"));


                        tLName->setText(i18n("<qt><b>%1</b></qt>").arg(KgpgInterface::checkForUtf8(kname).replace(QRegExp("<"),"&lt;")));
                        tLAlgo->setText(algo);
                }
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
        lEFinger->setText(fingervalue);


        if (isphoto) {
                kgpginfotmp=new KTempFile();
                kgpginfotmp->setAutoDelete(true);
                QString popt="cp %i "+kgpginfotmp->name();
                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<tid;
                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotinfoimgread(KProcess *)));
                p->start(KProcess::NotifyOnExit,true);
        }
        connect(buttonOk,SIGNAL(clicked()),this,SLOT(slotPreOk1()));
        connect(buttonPass,SIGNAL(clicked()),this,SLOT(slotChangePass()));
}

void KgpgKeyInfo::slotinfoimgread(KProcess *)
{
        QPixmap pixmap;
        pixmap.load(kgpginfotmp->name());
        pLPhoto->setPixmap(pixmap);
        kgpginfotmp->unlink();
}

void KgpgKeyInfo::slotChangePass()
{
        KgpgInterface *ChangeKeyPassProcess=new KgpgInterface();
        ChangeKeyPassProcess->KgpgChangePass(displayedKeyID);
}

void KgpgKeyInfo::slotPreOk1()
{
        if (expirationDate!=kDateWidget->date() || (isUnlimited!=cBExpiration->isChecked())) {
                KgpgInterface *KeyExpirationProcess=new KgpgInterface();
                KeyExpirationProcess->KgpgKeyExpire(displayedKeyID,kDateWidget->date(),cBExpiration->isChecked());
                connect(KeyExpirationProcess,SIGNAL(expirationFinished(int)),this,SLOT(slotPreOk2(int)));
        } else
                slotPreOk2(0);
}

void KgpgKeyInfo::slotPreOk2(int)
{
        if (ownerTrust!=kCOwnerTrust->currentText()) {
                KgpgInterface *KeyTrustProcess=new KgpgInterface();
                KeyTrustProcess->KgpgTrustExpire(displayedKeyID,kCOwnerTrust->currentText());
                connect(KeyTrustProcess,SIGNAL(trustfinished()),this,SLOT(accept()));
        } else
                accept();
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
        config->setGroup("General Options");
        QString defaultKeyID=KgpgInterface::getGpgSetting("default-key",config->readPathEntry("gpg config path"));


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
        if (KMessageBox::questionYesNo(this,i18n("<p>Do you want to import file <b>%1</b> into your key ring?</p>").arg(url.filename()))!=KMessageBox::Yes)
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
        QString keyid=currentItem()->text(5);
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



///////////////   main window for key management
listKeys::listKeys(QWidget *parent, const char *name, WFlags f) : KMainWindow(parent, name, f)//QDialog(parent,name,TRUE)//KMainWindow(parent, name)
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
        KAction *infoKey = new KAction(i18n("&Key Info"), "kgpg_info", Qt::Key_Return,this, SLOT(listsigns()),actionCollection(),"key_info");
        KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste),this, SLOT(slotPreImportKey()),actionCollection(),"key_import");
        setDefaultKey = new KAction(i18n("Set as De&fault Key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
        importSignatureKey = new KAction(i18n("Import Key From Keyserver"),"network", 0,this, SLOT(preimportsignkey()),actionCollection(),"key_importsign");
        importAllSignKeys = new KAction(i18n("Import Missing Signatures From Keyserver"),"network", 0,this, SLOT(importallsignkey()),actionCollection(),"key_importallsign");

        KStdAction::quit(this, SLOT(annule()), actionCollection());
        (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(),this, SLOT(refreshkey()),actionCollection(),"key_refresh");
        editKey = new KAction(i18n("&Edit Key"), "kgpg_edit", 0,this, SLOT(slotedit()),actionCollection(),"key_edit");
        KAction *exportSecretKey = new KAction(i18n("Export Secret Key..."), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
        KAction *deleteKeyPair = new KAction(i18n("Delete Key Pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
        KAction *generateKey = new KAction(i18n("&Generate Key Pair..."), "kgpg_gen", KStdAccel::shortcut(KStdAccel::New),this, SLOT(slotgenkey()),actionCollection(),"key_gener");
        KToggleAction *togglePhoto= new KToggleAction(i18n("&Show Photos"), "kgpg_photo", 0,this, SLOT(hidePhoto()),actionCollection(),"key_showp");
        (void) new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(keyserver()),actionCollection(),"key_server");
        KStdAction::preferences(this, SLOT(slotOptions()), actionCollection(),"kgpg_config");
        (void) new KAction(i18n("Tip of the &Day"), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
        (void) new KAction(i18n("View GnuPG Manual"), "contents", 0,this, SLOT(slotManpage()),actionCollection(),"gpg_man");
        KStdAction::keyBindings( this, SLOT( slotConfigureShortcuts() ),actionCollection(), "key_bind" );

        KStdAction::configureToolbars(this, SLOT(configuretoolbars() ), actionCollection(), "configuretoolbars");
        setStandardToolBarMenuEnabled(true);

        QVBoxLayout *vbox=new QVBoxLayout(page,3);

        keysList2->setRootIsDecorated(true);
        keysList2->addColumn( i18n( "Key" ) );
        keysList2->addColumn( i18n( "Trust" ) );
        keysList2->addColumn( i18n( "Expiration" ) );
        keysList2->addColumn( i18n( "Size" ) );
        keysList2->addColumn( i18n( "Creation" ) );
        keysList2->addColumn( i18n( "Id" ) );
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
        exportSecretKey->plug(popupsec);
        deleteKeyPair->plug(popupsec);

        popupout=new QPopupMenu();
        importKey->plug(popupout);
        generateKey->plug(popupout);

        popupsig=new QPopupMenu();
        importSignatureKey->plug(popupsig);
        delSignKey->plug(popupsig);

        keyPhoto=new QLabel(page);
        keyPhoto->setText(i18n("Photo"));
        keyPhoto->setFixedSize(60,60);
        keyPhoto->setScaledContents(true);
        keyPhoto->setFrameStyle( QFrame::Box | QFrame::Raised );

        vbox->addWidget(keysList2);
        //if (showPhoto==true)
        vbox->addWidget(keyPhoto);
        //vbox->addWidget(statusbar);
        setCentralWidget(page);

        QObject::connect(keysList2,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(listsigns()));
        QObject::connect(keysList2,SIGNAL(selectionChanged ()),this,SLOT(checkList()));
        QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                         this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));

        //QObject::connect(keysList2,SIGNAL(dropped(QDropEvent * , QListViewItem *)),this,SLOT(slotDroppedFile(QDropEvent * , QListViewItem *)));


        ///////////////    get all keys data
        createGUI("listkeys.rc");
        if (!configshowToolBar)
                toolBar()->hide();
        togglePhoto->setChecked(showPhoto);
        if (!showPhoto)
                keyPhoto->hide();
        else
                checkPhotos();
}


listKeys::~listKeys()
{
}


void listKeys::configuretoolbars()
{
        saveMainWindowSettings(KGlobal::config(), "Main Window");
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
        applyMainWindowSettings(KGlobal::config(), "Main Window");
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

void listKeys::hidePhoto()
{
        if (showPhoto) {
                keyPhoto->hide();
                showPhoto=false;
        } else {
                checkPhotos();
                showPhoto=true;
                displayPhoto();
                keyPhoto->show();
        }
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
        if (exportList.count()>1) {
                editKey->setEnabled(false);
                setDefaultKey->setEnabled(false);
                importAllSignKeys->setEnabled(false);
        } else {
                editKey->setEnabled(true);
                setDefaultKey->setEnabled(true);
                importAllSignKeys->setEnabled(true);
        }

        displayPhoto();
}

void listKeys::displayPhoto()
{
        if ((!showPhoto) || (keysList2->currentItem()==NULL))
                return;
        if (keysList2->currentItem()->depth()!=0) {
                keyPhoto->setText(i18n("No\nphoto"));
                return;
        }
        QString CurrentID=keysList2->currentItem()->text(5);
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
        //keysList2->currentItem()->setPixmap(1,pixmap);
        kgpgtmp->unlink();
}

void listKeys::annule()
{
        /////////  cancel & close window
        //exit(0);
        config->setGroup("General Options");
        config->writeEntry("show toolbar",toolBar()->isVisible());
        config->writeEntry("show photo",showPhoto);
        close();
        //reject();
}
/*
void listKeys::saveOptions()
{
  config->setGroup("General Options");
  config->writeEntry("default key",defaultkey);
}
*/


void listKeys::readOptions()
{
        config->setGroup("General Options");
        configshowToolBar=config->readBoolEntry("show toolbar",true);
        showPhoto=config->readBoolEntry("show photo",false);
        keysList2->displayMailFirst=config->readBoolEntry("display mail first",true);
        configUrl=config->readPathEntry("gpg config path");
        optionsDefaultKey=KgpgInterface::getGpgSetting("default-key",configUrl);
        QString defaultkey=optionsDefaultKey;
        if (!optionsDefaultKey.isEmpty())
                defaultkey.prepend("0x");
        config->writeEntry("default key",defaultkey);
        config->sync();
        if (config->readBoolEntry("selection clip",false)) {
                // support clipboard selection (if possible)
                if (kapp->clipboard()->supportsSelection())
                        kapp->clipboard()->setSelectionMode(true);
        } else
                kapp->clipboard()->setSelectionMode(false);
        keysList2->defKey=defaultkey;
}


void listKeys::slotOptions()
{
        kgpgOptions *opts=new kgpgOptions(this);
        opts->exec();
        delete opts;
        readOptions();
        refreshkey();
}

void listKeys::slotSetDefKey()
{

        if (keysList2->currentItem()->pixmap(1)->serialNumber()!=keysList2->trustgood.serialNumber()) {
                KMessageBox::sorry(this,i18n("Sorry, this key is not valid for encryption or not trusted."));
                return;
        }

        /////////////// revert old default key to normal icon
        keysList2->defKey=keysList2->currentItem()->text(5);
        refreshkey();

        QListViewItem *olddef = keysList2->firstChild();
        while (olddef->text(5)!=keysList2->defKey)
                if (olddef->nextSibling())
                        olddef = olddef->nextSibling();
                else
                        break;
        keysList2->setSelected(olddef,true);

        ////////////// store new default key

        config->setGroup("General Options");
        config->writeEntry("default key",keysList2->defKey);
        KgpgInterface::setGpgSetting("default-key",keysList2->defKey.right(keysList2->defKey.length()-2),configUrl);
}

void listKeys::slotstatus(QListViewItem *)
{
        ////////////  echo key email in statusbar
        /*
        if (sel->depth()!=0) statusbar->message("");
        else
        {
        QStringList result=keynames.grep(sel->text(5));
        QString res=result.join("");
        res=res.section(',',1,1);
        statusbar->message(res);
        }
        */
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
                        if ((sel->text(3)=="-") && (sel->text(5)!="-")) {
                                if ((sel->text(1)=="-") || (sel->text(1)==i18n("Revoked"))) {
                                        if (sel->text(0).find(i18n("User id not found"))==-1)
                                                importSignatureKey->setEnabled(false);
                                        else
                                                importSignatureKey->setEnabled(true);
                                        popupsig->exec(pos);
                                        return;
                                }
                        }
                } else {
                        keysList2->setSelected(sel,TRUE);
                        if ((keysList2->secretList.find(sel->text(5))!=-1) && (keysList2->selectedItems().count()==1))
                                popupsec->exec(pos);
                        else
                                popup->exec(pos);
                        return;
                }
        } else
                popupout->exec(pos);
}


void listKeys::slotexportsec()
{
        //////////////////////   export secret key
        QString warn=i18n("Secret keys SHOULD NOT be saved in an unsafe place.\n"
                          "If someone else can access this file, encryption with this key will be compromised!\nContinue key export?");
        int result=KMessageBox::questionYesNo(this,warn,i18n("Warning"));
        if (result!=KMessageBox::Yes)
                return;

        QString key=keysList2->currentItem()->text(0);

        QString sname=key.section('@',0,0);
        sname=sname.section(' ',-1,-1);
        sname=sname.section('.',0,0);
        sname=sname.section('(',-1,-1);
        sname.append(".asc");
        sname.prepend(QDir::homeDirPath()+"/");
        KURL url=KFileDialog::getSaveURL(sname,"*.asc|*.asc Files", this, i18n("Export PRIVATE KEY As"));

        if(!url.isEmpty()) {
                QFile fgpg(url.path());
                if (fgpg.exists())
                        fgpg.remove();

                KProcIO *p=new KProcIO();
                *p<<"gpg"<<"--no-tty"<<"--output"<<QFile::encodeName(url.path())<<"--armor"<<"--export-secret-keys"<<keysList2->currentItem()->text(5);
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
                QString key=keysList2->currentItem()->text(0);
                sname=key.section('@',0,0);
                sname=sname.section(' ',-1,-1);
                sname=sname.section('.',0,0);
                sname=sname.section('(',-1,-1);
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
                                                *p<<(exportList.at(i)->text(5)).stripWhiteSpace();


                                p->start(KProcess::Block);
                                if (fgpg.exists())
                                        KMessageBox::information(this,i18n("Your public key \"%1\" was successfully exported\n").arg(expname));
                                else
                                        KMessageBox::sorry(this,i18n("Your public key could not be exported\nCheck the key."));
                        }
                } else {
                        message="";
                        *p<<"--export"<<"--armor";
                        if (!exportAttr)
                                *p<<"--export-options"<<"no-include-attributes";

                        for ( uint i = 0; i < exportList.count(); ++i )
                                if ( exportList.at(i) )
                                        *p<<(exportList.at(i)->text(5)).stripWhiteSpace();

                        if (dial->checkClipboard->isChecked())
                                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessExportClip(KProcess *)));
                        else
                                QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessExportMail(KProcess *)));
                        QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(slotReadProcess(KProcIO *)));
                        p->start(KProcess::NotifyOnExit,false);
                }
        }
        delete dial;
}

void listKeys::slotReadProcess(KProcIO *p)
{
        QString outp;
        while (p->readln(outp)!=-1)
                message+=outp+"\n";
}


void listKeys::slotProcessExportMail(KProcess *)
{
        ///////////////////////// send key by mail
        KProcIO *proc=new KProcIO();
        QString subj="Public key:";
        *proc<<"kmail"<<"--subject"<<subj<<"--body"<<message;
        proc->start(KProcess::DontCare);
}

void listKeys::slotProcessExportClip(KProcess *)
{
        // if (kapp->clipboard()->supportsSelection())
        //   kapp->clipboard()->setSelectionMode(true);
        kapp->clipboard()->setText(message);
}



void listKeys::listsigns()
{
        if (keysList2->currentItem()==NULL)
                return;
        if (keysList2->currentItem()->depth()!=0) {
                if (keysList2->currentItem()->text(0)==i18n("Photo Id")) {
                        //////////////////////////    display photo
                        KProcIO *p=new KProcIO();
                        QString popt="kview %i";
                        *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<keysList2->currentItem()->parent()->text(5);
                        p->start(KProcess::DontCare,true);
                        return;
                } else
                        return;
        }

        /////////////   open a key info dialog (KgpgKeyInfo, see begining of this file)
        QString key=keysList2->currentItem()->text(5);
        if (key!="") {
                QColor pix;
                if (keysList2->currentItem()->pixmap(1)->serialNumber()==keysList2->trustgood.serialNumber())
                        pix.setRgb(148,255,0);//;pix=keysList2->trustgood;
                else if (keysList2->currentItem()->pixmap(1)->serialNumber()==keysList2->trustunknown.serialNumber())
                        pix.setRgb(255,255,255); //pix=keysList2->trustunknown;
                else
                        pix.setRgb(172,0,0); //keysList2->trustbad;
                bool isSecret=false;
                if (keysList2->secretList.find(keysList2->currentItem()->text(5))!=-1)
                        isSecret=true;
                KgpgKeyInfo *opts=new KgpgKeyInfo(this,"key_props",key, pix,isSecret);
                opts->exec();
                delete opts;
                keysList2->refreshcurrentkey(keysList2->currentItem());
        }
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
                QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --fingerprint "+KShellProcess::quote(keysList2->currentItem()->text(5));
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
                          "is not trying to intercept your communications</qt>").arg(keysList2->currentItem()->text(0)).arg(keysList2->currentItem()->text(5)).arg(fingervalue);

                if (KMessageBox::warningContinueCancel(this,opt)!=KMessageBox::Continue)
                        return;

        } else {
                QStringList signKeyList;
                for ( uint i = 0; i < signList.count(); ++i )
                        if ( signList.at(i) )
                                signKeyList+=signList.at(i)->text(0)+": "+signList.at(i)->text(5);
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
                        signKeyProcess->KgpgSignKey(signList.at(globalCount)->text(5),globalkeyID,globalkeyMail,globalisLocal,globalChecked);
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
                KMessageBox::sorry(this,i18n("<qt>Bad passphrase, key <b>%1</b> not signed.</qt>").arg(signList.at(globalCount-1)->text(0)));
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
                        missingKeysList+=current->text(5)+" ";
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
                importsignkey(keysList2->currentItem()->text(5));
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
        parentKey=keysList2->currentItem()->parent()->text(5);
        signID=keysList2->currentItem()->text(5);
        parentMail=keysList2->currentItem()->parent()->text(0);
        signMail=keysList2->currentItem()->text(0);

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
        <<keysList2->currentItem()->text(5)
        <<"help";
        kp.start(KProcess::Block);
        refreshkey();
}


void listKeys::slotgenkey()
{
        //////////  generate key
        keyGenerate *genkey=new keyGenerate(this,0);
        if (genkey->exec()==QDialog::Accepted) {
                if (genkey->getmode()==false)   ///  normal mode
                {
                        //// extract data
                        QString ktype=genkey->getkeytype();
                        QString ksize=genkey->getkeysize();
                        int kexp=genkey->getkeyexp();
                        QString knumb=genkey->getkeynumb();
                        QString kname=genkey->getkeyname();
                        QString kmail=genkey->getkeymail();
                        QString kcomment=genkey->getkeycomm();
                        delete genkey;

                        //genkey->delayedDestruct();
                        QCString password;
                        bool goodpass=false;
                        while (!goodpass)
                        {
                                int code=KPasswordDialog::getNewPassword(password,i18n("<b>Enter passphrase for %1</b>:<br>Passphrase should include non alphanumeric characters and random sequences").arg(kmail));
                                if (code!=QDialog::Accepted)
                                        return;
                                if (password.length()<5)
                                        KMessageBox::sorry(this,i18n("This passphrase is not secure enough.\nMinimum length= 5 characters"));
                                else
                                        goodpass=true;
                        }

                        pop = new QDialog( this,0,false,WStyle_Customize | WStyle_NormalBorder);
                        QVBoxLayout *vbox=new QVBoxLayout(pop,3);
                        QLabel *tex=new QLabel(pop);
                        tex->setText(i18n("Generating new key pair. Please wait."));
                        vbox->addWidget(tex);
                        pop->adjustSize();
                        pop->show();

                        KProcIO *proc=new KProcIO();

                        //*proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--passphrase-fd"<<res<<"--gen-key"<<"-a"<<"kgpg.tmp";
                        *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--gen-key";
                        /////////  when process ends, update dialog infos
                        QObject::connect(proc, SIGNAL(processExited(KProcess *)),this, SLOT(genover(KProcess *)));
                        proc->start(KProcess::NotifyOnExit);

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
                        proc->writeStdin(QString("Name-Real:%1").arg(kname.utf8()));
                        //proc->writeStdin(QTextCodec::fromUnicode("Name-Real:Cortès"));
                        proc->writeStdin(QString("Name-Email:%1").arg(kmail));
                        if (kcomment!="")
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
                        proc->writeStdin("EOF");
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

void listKeys::genover(KProcess *)
{
        delete pop;
        refreshkey();
}


void listKeys::deleteseckey()
{
        //////////////////////// delete a key
        QString res=keysList2->currentItem()->text(0);
        res.replace(QRegExp("<"),"&lt;");
        int result=KMessageBox::questionYesNo(this,
                                              i18n("<p>Delete <b>SECRET KEY</b> pair <b>%1</b> ?</p>Deleting this key pair means you will never be able to decrypt files encrypted with this key anymore!").arg(res),
                                              i18n("Warning"),
                                              i18n("Delete"));
        if (result!=KMessageBox::Yes)
                return;

        KProcess *conprocess=new KProcess();
        *conprocess<< "konsole"<<"-e"<<"gpg"
        <<"--no-secmem-warning"
        <<"--delete-secret-key"<<keysList2->currentItem()->text(5);
        QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(deletekey()));
        conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::confirmdeletekey()
{
        if ((keysList2->secretList.find(keysList2->currentItem()->text(5))!=-1) && (keysList2->selectedItems().count()==1))
                deleteseckey();
        else {
                QStringList keysToDelete;
                QString secretList;
                QPtrList<QListViewItem> exportList=keysList2->selectedItems();
                bool secretKeyInside=false;
                for ( uint i = 0; i < exportList.count(); ++i )
                        if ( exportList.at(i) ) {
                                if (keysList2->secretList.find(exportList.at(i)->text(5))!=-1) {
                                        secretKeyInside=true;
                                        secretList+=(exportList.at(i)->text(0)).replace(QRegExp("<"),"&lt;")+"<br>";
                                        exportList.at(i)->setSelected(false);
                                } else
                                        keysToDelete+=(exportList.at(i)->text(0)).replace(QRegExp("<"),"&lt;");
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
                        gp<<(exportList.at(i)->text(5)).stripWhiteSpace();

        gp.start(KProcess::Block);

        for ( uint i = 0; i < exportList.count(); ++i )
                if ( exportList.at(i) )
                        delete exportList.at(i);

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

void KeyView::expandKey(QListViewItem *item)
{
        if (item->childCount()!=0)
                return;   // key has already been expanded
        FILE *fp;
        QString tst,cycle,revoked;
        char line[300];
        SmallViewItem *itemsub=NULL;
        SmallViewItem *itemuid=NULL;
        SmallViewItem *itemsig=NULL;
        cycle="pub";
        bool noID=false;

        fp = popen(QFile::encodeName(QString("gpg --no-secmem-warning --no-tty --with-colon --list-sigs "+item->text(5))), "r");

        while ( fgets( line, sizeof(line), fp)) {
                tst=line;

                if (tst.startsWith("uid") || tst.startsWith("uat")) {
                        gpgKey uidKey=extractKey(tst);

                        //          QString tr=trustString(trust).gpgkeytrust;
                        if (tst.startsWith("uat")) {
                                itemuid= new SmallViewItem(item,i18n("Photo Id"),"","-","-","-","-");
                                itemuid->setPixmap(0,pixuserphoto);
                                itemuid->setPixmap(1,uidKey.trustpic);
                                cycle="uid";
                        } else {
                                itemuid= new SmallViewItem(item,extractKeyName(uidKey.gpgkeyname,uidKey.gpgkeymail),"","-","-","-","-");
                                itemuid->setPixmap(1,uidKey.trustpic);
                                if (noID) {
                                        item->setText(0,extractKeyName(uidKey.gpgkeyname,uidKey.gpgkeymail));
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

                                        QString fsigname=extractKeyName(sigKey.gpgkeyname,sigKey.gpgkeymail);
                                        if (tst.section(':',10,10).endsWith("l"))
                                                fsigname+=i18n(" [local]");

                                        if (cycle=="pub")
                                                itemsig= new SmallViewItem(item,fsigname,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="sub")
                                                itemsig= new SmallViewItem(itemsub,fsigname,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);
                                        if (cycle=="uid")
                                                itemsig= new SmallViewItem(itemuid,fsigname,"-",sigKey.gpgkeyexpiration,"-",sigKey.gpgkeycreation,sigKey.gpgkeyid);

                                        itemsig->setPixmap(0,pixsignature);
                                } else

                                        if (tst.startsWith("sub")) {
                                                gpgKey subKey=extractKey(tst);
                                                tst=i18n("%1 subkey").arg(subKey.gpgkeyalgo);
                                                itemsub= new SmallViewItem(item,tst,"",subKey.gpgkeyexpiration,subKey.gpgkeysize,subKey.gpgkeycreation,subKey.gpgkeyid);
                                                itemsub->setPixmap(0,pixkeySingle);
                                                itemsub->setPixmap(1,subKey.trustpic);
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
                        if (currentRevoke.find(current->text(5))!=-1)
                        {
                                current->setText(1,i18n("Revoked"));
                                found=true;
                        }

                QListViewItem *subcurrent = current->firstChild();
                if (subcurrent)
                {
                        if (currentRevoke.find(subcurrent->text(5))!=-1) {
                                subcurrent->setText(1,i18n("Revoked"));
                                found=true;
                        }
                        while (subcurrent->nextSibling()) {
                                subcurrent = subcurrent->nextSibling();
                                if (currentRevoke.find(subcurrent->text(5))!=-1) {
                                        subcurrent->setText(1,i18n("Revoked"));
                                        found=true;
                                }
                        }
                }

                while ( current->nextSibling() )
                {
                        current = current->nextSibling();
                        if (currentRevoke.find(current->text(5))!=-1) {
                                current->setText(1,i18n("Revoked"));
                                found=true;
                        }

                        QListViewItem *subcurrent = current->firstChild();
                        if (subcurrent) {
                                if (currentRevoke.find(subcurrent->text(5))!=-1) {
                                        subcurrent->setText(1,i18n("Revoked"));
                                        found=true;
                                }
                                while (subcurrent->nextSibling()) {
                                        subcurrent = subcurrent->nextSibling();
                                        if (currentRevoke.find(subcurrent->text(5))!=-1) {
                                                subcurrent->setText(1,i18n("Revoked"));
                                                found=true;
                                        }
                                }
                        }
                }
                if (!found)
                        (void) new SmallViewItem(item,i18n("Revocation Certificate"),"+","+","+","+",currentRevoke);
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

        // get current position.
        int colWidth = QMAX(150, columnWidth(0));
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
        fp2 = popen("gpg --no-secmem-warning --no-tty --list-secret-keys", "r");
        while ( fgets( line, sizeof(line), fp2))
                issec+=line;
        pclose(fp2);

        fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8", "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
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


                        item=new UpdateViewItem(this,extractKeyName(pubKey.gpgkeyname,pubKey.gpgkeymail),"",pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold,isexpired);

                        item->setPixmap(1,pubKey.trustpic);

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

        if(current != NULL) {
                // select previous selected
                QListViewItem *newPos = findItem(current->text(0), 0);
                setSelected(newPos, true);
                ensureItemVisible(newPos);
                delete current;
        }

        if (columnWidth(0) > colWidth)
                setColumnWidth(0, colWidth);
}

void KeyView::refreshcurrentkey(QListViewItem *current)
{
        if (current==NULL)
                return;
        //KMessageBox::sorry(0,current->text(5));
        ////////   update display of the current key
        FILE *fp;
        QString tst,cycle,revoked;
        char line[300];
        QString cmd="gpg --no-secmem-warning --no-tty --with-colon --list-keys --charset utf8 "+current->text(5);
        fp = popen(QFile::encodeName(cmd), "r");
        while ( fgets( line, sizeof(line), fp)) {
                tst=line;
                if (tst.startsWith("pub")) {
                        gpgKey pubKey=extractKey(tst);
                        current->setText(2,pubKey.gpgkeyexpiration);
                        current->setPixmap(1,pubKey.trustpic);
                        current->repaint();
                }
        }
        pclose(fp);
}



QString KeyView::extractKeyName(QString name,QString mail)
{
        name=KgpgInterface::checkForUtf8(name);
        if (displayMailFirst)
                return QString(mail+" ("+name+")");
        return QString(name+" ("+mail+")");
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
