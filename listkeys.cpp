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

////////////////////////////////////////////////////// code for the key managment

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

#include <kio/netaccess.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kshortcut.h>
#include <kstdaccel.h>

#include "listkeys.h"
#include "kgpginterface.h"
#include "keyservers.h"

//////////////  KListviewItem special

class UpdateViewItem : public KListViewItem
{
public:
  UpdateViewItem(QListView *parent, QString tst, QString tr, QString val, QString size, QString creat, QString id,QString defaultkey);
  bool Itemdefault;
  virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
};

UpdateViewItem::UpdateViewItem(QListView *parent, QString tst, QString tr, QString val, QString size, QString creat, QString id,QString defaultkey)
    : KListViewItem(parent)
{
  if (defaultkey=="on")
    Itemdefault=true;
  else
    Itemdefault=false;
  setText(0,tst);
  setText(1,tr);
  setText(2,val);
  setText(3,size);
  setText(4,creat);
  setText(5,id);
}


void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
  if ((Itemdefault) && (column==0))
  {
    QFont font(p->font());
    font.setBold(true);
    p->setFont(font);
  }
  KListViewItem::paintCell(p, cg, column, width, alignment);
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
  if (column==0)
  {
    QFont font(p->font());
    //font.setPointSize(font.pointSize()-1);
    font.setItalic(true);
    p->setFont(font);
  }
  KListViewItem::paintCell(p, cg, column, width, alignment);
}


////////  window for the key info dialog
KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name,QString sigkey):KDialogBase( parent, name, true,sigkey,Close)
{
  QString message,fingervalue;
  resize(450,100);
  QWidget *page=new QWidget(this);
  QLabel *labelname = new QLabel(page);
  QLabel *labelmail = new QLabel(page);
  QLabel *labeltype = new QLabel(page);
  QLabel *labellength = new QLabel(page);
  QLabel *labelcreation = new QLabel(page);
  QLabel *labelexpire = new QLabel(page);
  QLabel *labeltrust = new QLabel(page);
  QLabel *labelid = new QLabel(page);
  QLabel *labelcomment = new QLabel(page);
  QLabel *labelfinger = new QLabel(i18n("Fingerprint:"),page);
  //QString labelname,labelmail,labeltype,labellength,labelfinger,labelcreation,labelexpire,labeltrust,labelid,fingervalue;
  //QVBoxLayout *vbox=new QVBoxLayout(page,2);


  FILE *pass;
  char line[200]="";
  QString opt,tid;
  bool isphoto=false;
  QString gpgcmd="gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-key "+KShellProcess::quote(sigkey.local8Bit());

  pass=popen(gpgcmd,"r");
  while ( fgets( line, sizeof(line), pass))
  {
    opt=line;
    if (opt.startsWith("uat"))
      isphoto=true;
    if (opt.startsWith("pub"))
    {
      QString algo=opt.section(':',3,3);

      switch( algo.toInt() )
      {
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
      switch( trust[0] )
      {
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
      QString id=QString("0x"+tid.right(8));

      QString fullname=opt.section(':',9,9);
      if (opt.section(':',6,6)=="")

        labelexpire->setText(i18n("Expiration: never"));
      else
        labelexpire->setText(i18n("Expiration: ")+opt.section(':',6,6));
      labelcreation->setText(i18n("Creation: ")+opt.section(':',5,5));
      labelname=new QLabel(i18n("Name: ")+fullname.section('<',0,0),page);
      labellength->setText(i18n("Length: ")+opt.section(':',2,2));
      labeltrust->setText(i18n("Trust: %1").arg(tr));
      labelid->setText(i18n("Id: ")+tid);
      if (fullname.find("<")!=-1)
      {
        QString kmail=fullname.section('<',-1,-1);
        kmail.truncate(kmail.length()-1);
        labelmail->setText(i18n("Email: %1").arg(kmail));
      }
      else labelmail->setText(i18n("Email: none"));

      QString kname=fullname.section('<',0,0);
      if (fullname.find("(")!=-1)
      {
        kname=kname.section('(',0,0);
        QString comment=fullname.section('(',1,1);
        comment=comment.section(')',0,0);
        labelcomment->setText(i18n("Comment: %1").arg(comment));
      }
      else labelcomment->setText(i18n("Comment: none"));

      labelname->setText(i18n("Name: %1").arg(kname));
      labeltype->setText(i18n("Algorithm: %1").arg(algo));
    }
    if (opt.startsWith("fpr"))
    {
      fingervalue=opt.section(':',9,9);
      // format fingervalue in 4-digit groups
      uint len = fingervalue.length();
      if ((len > 0) && (len % 4 == 0))
        for (uint n = 0; 4*(n+1) < len; n++)
          fingervalue.insert(5*n+4, ' ');
    }
  }
  pclose(pass);


  QGridLayout *Form1Layout = new QGridLayout( page, 1, 1, 11, 6, "Form1Layout");

  Form1Layout->addWidget( labelid, 8, 1 );

  Form1Layout->addWidget( labeltrust, 7, 1 );

  Form1Layout->addWidget( labelexpire, 6, 1 );

  Form1Layout->addWidget( labelcreation, 5, 1 );

  Form1Layout->addWidget( labellength, 4, 1 );

  Form1Layout->addWidget( labeltype, 3, 1 );

  Form1Layout->addWidget( labelcomment, 2, 1 );

  Form1Layout->addWidget( labelmail, 1, 1 );

  Form1Layout->addWidget( labelname, 0, 1 );

  Form1Layout->addWidget( labelfinger, 9, 1 );

  KLineEdit *finger=new KLineEdit("",page);
  finger->setReadOnly(true);
  finger->setPaletteBackgroundColor(QColor(white));
  finger->setText(fingervalue);
  Form1Layout->addMultiCellWidget( finger, 10, 10, 0, 1 );

  keyinfoPhoto=new QLabel(i18n("No photo"),page);
  //keyinfoPhoto->setFixedSize(70,80);
  //keyinfoPhoto->setScaledContents(true);
  keyinfoPhoto->setAlignment(Qt::AlignTop);

  keyinfoPhoto->setFrameStyle( QFrame::Box | QFrame::Raised );

  if (isphoto)
  {
    kgpginfotmp=new KTempFile();
    kgpginfotmp->setAutoDelete(true);
    QString popt="cp %i "+kgpginfotmp->name();
    KProcIO *p=new KProcIO();
    *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<tid;
    QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotinfoimgread(KProcess *)));
    //QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(slotinfoimgread(KProcIO *)));
    p->start(KProcess::NotifyOnExit,true);
  }

  Form1Layout->addMultiCellWidget( keyinfoPhoto, 0, 9, 0, 0 );

  setMainWidget(page);
  page->show();

}

void KgpgKeyInfo::slotinfoimgread(KProcess *)//IO *p)
{
  QPixmap pixmap;
  pixmap.load(kgpginfotmp->name());
  keyinfoPhoto->setPixmap(pixmap);
  kgpginfotmp->unlink();
}


////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////   Secret key selection dialog, used when user wants to sign a key
KgpgSelKey::KgpgSelKey(QWidget *parent, const char *name,bool showlocal):KDialogBase( parent, name, true,i18n("Private Key List"),Ok | Cancel)
{
  QString keyname;
  QWidget *page = new QWidget(this);
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
  QVBoxLayout *vbox=new QVBoxLayout(page,3);

  vbox->addWidget(labeltxt);
  vbox->addWidget(keysListpr);
  if (showlocal==true)
  {
    local = new QCheckBox(i18n("Local signature (cannot be exported)"),page);
    vbox->addWidget(local);
  }

  FILE *fp,*fp2;
  QString tst,tst2;
  char line[130];
  fp = popen(QString("gpg --no-tty --with-colon --list-secret-keys"), "r");
  while ( fgets( line, sizeof(line), fp))
  {
    tst=line;
    if (tst.startsWith("sec"))
    {
      const QString trust=tst.section(':',1,1);
      QString val=tst.section(':',6,6);
      QString id=QString("0x"+tst.section(':',4,4).right(8));
      if (val=="")
        val=i18n("Unlimited");
      QString tr;
      switch( trust[0] )
      {
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

      fp2 = popen(QString("gpg --no-tty --with-colon --list-key %1").arg(KShellProcess::quote(id)), "r");
      bool dead=true;
      while ( fgets( line, sizeof(line), fp2))
      {
        tst2=line;
        if (tst2.startsWith("pub"))
        {
          const QString trust2=tst2.section(':',1,1);
          switch( trust2[0] )
          {
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
      if (!tst.isEmpty() && (!dead))
      {
        KListViewItem *item=new KListViewItem(keysListpr,extractKeyName(tst));
        KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, expiration: %3").arg(id).arg(tr).arg(val));
        sub->setSelectable(false);
        item->setPixmap(0,keyPair);
      }
    }
  }
  pclose(fp);


  QObject::connect(keysListpr,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(slotpreOk()));
  QObject::connect(keysListpr,SIGNAL(clicked(QListViewItem *)),this,SLOT(slotSelect(QListViewItem *)));


  keysListpr->setSelected(keysListpr->firstChild(),true);

  page->show();
  resize(this->minimumSize());
  setMainWidget(page);
}

QString KgpgSelKey::extractKeyName(QString fullName)
{
  QString kMail;
  if (fullName.find("<")!=-1)
  {
    kMail=fullName.section('<',-1,-1);
    kMail.truncate(kMail.length()-1);
  }
  QString kName=fullName.section('<',0,0);
  if (kName.find("(")!=-1) kName=kName.section('(',0,0);
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
  if (item==NULL) return;
  if (item->depth()!=0)
  {
    keysListpr->setSelected(item->parent(),true);
    keysListpr->setCurrentItem(item->parent());
  }
}


QString KgpgSelKey::getkeyID()
{
  QString userid;
  /////  emit selected key
  if (keysListpr->currentItem()==NULL) return("");
  else
  {
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
  if (keysListpr->currentItem()==NULL) return("");
  else
  {
    username=keysListpr->currentItem()->text(0);
    //username=username.section(' ',0,0);
    username=username.stripWhiteSpace();
    return(username);
  }
}

bool KgpgSelKey::getlocal()
{
  /////  emit exportation choice
  return(local->isChecked());
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



  setAcceptDrops(true);
  setDragEnabled(true);
}

void  KeyView::droppedfile (KURL url)
{
  if (KMessageBox::questionYesNo(this,i18n("<p>Do you want to import file <b>%1</b> into your key ring ?</p>").arg(url.filename()))!=KMessageBox::Yes)
    return;

  KgpgInterface *importKeyProcess=new KgpgInterface();
  importKeyProcess->importKeyURL(url);
  connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkeylist()));
}

void KeyView::contentsDragMoveEvent(QDragMoveEvent *e)
{
  e->accept (QUrlDrag::canDecode(e));
}

void  KeyView::contentsDropEvent (QDropEvent *o)
{
  QStringList list;
  if ( QUrlDrag::decodeToUnicodeUris( o, list ) ) droppedfile(KURL(list.first()));
}

void  KeyView::startDrag()
{
  FILE *fp;
  char line[200]="";
  QString keyid=currentItem()->text(5);
  if (!keyid.startsWith("0x")) return;
  QString gpgcmd="gpg --no-tty --export --armor "+KShellProcess::quote(keyid.local8Bit());

  QString keytxt;
  fp=popen(gpgcmd,"r");
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
  config=kapp->config();
  readOptions();
  //if (enctodef==true) defKey=defaultKey;
  //else defKey="";
  setCaption(i18n("Key Management"));
  //(void) new KAction(i18n("&Refresh List"), KStdAccel::shortcut(KStdAccel::Copy),this, SLOT(slotexport()),actionCollection(),"key_copy");
  KAction *exportPublicKey = new KAction(i18n("E&xport Public Key..."), "kgpg_export", KStdAccel::shortcut(KStdAccel::Copy),this, SLOT(slotexport()),actionCollection(),"key_export");
  KAction *deleteKey = new KAction(i18n("&Delete Key"),"editdelete", Qt::Key_Delete,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
  KAction *signKey = new KAction(i18n("&Sign Key..."), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
  KAction *delSignKey = new KAction(i18n("Delete Sign&ature"),0, 0,this, SLOT(delsignkey()),actionCollection(),"key_delsign");
  KAction *infoKey = new KAction(i18n("&Key Info"), "kgpg_info", Qt::Key_Return,this, SLOT(listsigns()),actionCollection(),"key_info");
  KAction *importKey = new KAction(i18n("&Import Key..."), "kgpg_import", KStdAccel::shortcut(KStdAccel::Paste),this, SLOT(slotPreImportKey()),actionCollection(),"key_import");
  KAction *setDefaultKey = new KAction(i18n("Set as De&fault Key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
KAction *importSignKey = new KAction(i18n("Import Key from Keyserver"),0, 0,this, SLOT(importsignkey()),actionCollection(),"key_importsign");

  KStdAction::quit(this, SLOT(annule()), actionCollection());
  (void) new KAction(i18n("&Refresh List"), "reload", KStdAccel::reload(),this, SLOT(refreshkey()),actionCollection(),"key_refresh");
  KAction *editKey = new KAction(i18n("&Edit Key"), "kgpg_edit", 0,this, SLOT(slotedit()),actionCollection(),"key_edit");
  KAction *exportSecretKey = new KAction(i18n("Export Secret Key..."), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
  KAction *deleteKeyPair = new KAction(i18n("Delete Key Pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
  KAction *generateKey = new KAction(i18n("&Generate Key Pair..."), "kgpg_gen", KStdAccel::shortcut(KStdAccel::New),this, SLOT(slotgenkey()),actionCollection(),"key_gener");
  KToggleAction *togglePhoto= new KToggleAction(i18n("&Show Photos"), "kgpg_photo", 0,this, SLOT(hidePhoto()),actionCollection(),"key_showp");
  (void) new KAction(i18n("&Key Server Dialog"), "network", 0,this, SLOT(keyserver()),actionCollection(),"key_server");
  KStdAction::preferences(this, SLOT(slotOptions()), actionCollection());
  (void) new KAction(i18n("Tip of the &Day"), "idea", 0,this, SLOT(slotTip()), actionCollection(),"help_tipofday");
  (void) new KAction(i18n("View GnuPG Manual"), "contents", 0,this, SLOT(slotManpage()),actionCollection(),"gpg_man");


  //KStdAction::preferences(this, SLOT(slotParentOptions()), actionCollection());
#if (KDE_VERSION >= 310)
  (void) new KToggleToolBarAction("mainToolBar",i18n("Show Toolbar"), actionCollection(),"pref_toolbar"); ///  KDE 3.1 only
#endif

  QVBoxLayout *vbox=new QVBoxLayout(page,3);
  //keysList2 = new KListView(page);

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

  popup=new QPopupMenu();
  exportPublicKey->plug(popup);
  deleteKey->plug(popup);
  signKey->plug(popup);
  infoKey->plug(popup);
  editKey->plug(popup);
  setDefaultKey->plug(popup);

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
  delSignKey->plug(popupsig);
  importSignKey->plug(popupsig);

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
  QObject::connect(keysList2,SIGNAL(selectionChanged ()),this,SLOT(displayPhoto()));
  QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                   this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));
  //QObject::connect(keysList2,SIGNAL(dropped(QDropEvent * , QListViewItem *)),this,SLOT(slotDroppedFile(QDropEvent * , QListViewItem *)));


  ///////////////    get all keys data
  refreshkey();
  createGUI("listkeys.rc");
  if (!configshowToolBar) toolBar()->hide();
  togglePhoto->setChecked(showPhoto);
  if (!showPhoto) keyPhoto->hide();
}


listKeys::~listKeys()
{}

void listKeys::slotManpage()
{
   kapp->startServiceByDesktopName("khelpcenter", QString("man:/gpg"), 0, 0, 0, "", true);
}

void listKeys::slotTip()
{
KTipDialog::showTip(this, "kgpg/tips", true);
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
  delete ks;
  refreshkey();
}

void listKeys::hidePhoto()
{
  if (showPhoto)
  {
    keyPhoto->hide();
    showPhoto=false;
  }
  else
  {
    showPhoto=true;
    displayPhoto();
    keyPhoto->show();
  }
}

void listKeys::displayPhoto()
{
  if ((!showPhoto) || (keysList2->currentItem()==NULL))
    return;
  if (keysList2->currentItem()->depth()!=0)
  {
    keyPhoto->setText(i18n("No\nphoto"));
    return;
  }
  QString CurrentID=keysList2->currentItem()->text(5);
  if (keysList2->photoKeysList.find(CurrentID)!=-1)
  {
    kgpgtmp=new KTempFile();
    QString popt="cp %i "+kgpgtmp->name();
    KProcIO *p=new KProcIO();
    *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<CurrentID;
    QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessPhoto(KProcess *)));
    //QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(slotinfoimgread(KProcIO *)));
    p->start(KProcess::NotifyOnExit,true);
  }
  else
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
  bool encrypttodefault=config->readBoolEntry("encrypt to default key",false);
  configshowToolBar=config->readBoolEntry("show toolbar",true);
  showPhoto=config->readBoolEntry("show photo",false);
  keysList2->displayMailFirst=config->readBoolEntry("display mail first",true);
  QString defaultkey=config->readEntry("default key");
  if (config->readBoolEntry("selection clip",false))
  {
    // support clipboard selection (if possible)
    if (kapp->clipboard()->supportsSelection())
      kapp->clipboard()->setSelectionMode(true);
  }
  else kapp->clipboard()->setSelectionMode(false);

  if (encrypttodefault)
    keysList2->defKey=defaultkey;
  else
    keysList2->defKey="";
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
  QString block=i18n("Invalid")+" "+i18n("Disabled")+" "+i18n("Revoked")+" "+i18n("Expired")+" "+i18n("?");
  QString key=keysList2->currentItem()->text(5);

config->setGroup("General Options");
  if  (!config->readBoolEntry("encrypt to default key",false))
  {
    KMessageBox::sorry(this,i18n("Before setting a default key, you must enable default key encryption in the options dialog."));
    return;
  }

  if  (block.find(keysList2->currentItem()->text(1))!=-1)
  {
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
  if (sel!=NULL)
  {
    if (sel->depth()!=0)
    {
      if ((sel->text(1)=="-") && (sel->text(3)=="-"))
        popupsig->exec(pos);
      if ((sel->text(1)==i18n("Revoked")) && (sel->text(3)=="-"))
        popupsig->exec(pos);
      //else popupout->exec(pos);
    }
    else
    {
      keysList2->setSelected(sel,TRUE);
      if (keysList2->secretList.find(sel->text(5))!=-1)
        popupsec->exec(pos);
      else
        popup->exec(pos);
    }
  }
  else
    popupout->exec(pos);
}


void listKeys::slotexportsec()
{
  //////////////////////   export secret key
  QString warn=i18n("Secret keys SHOULD NOT be saved in an unsafe place.\n"
                    "If someone else can access this file, encryption with this key will be compromised!\nContinue key export?");
  int result=KMessageBox::warningYesNo(this,warn,i18n("Warning"));
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

  if(!url.isEmpty())
  {
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

  KURL u;
  QString key=keysList2->currentItem()->text(0);

  QString sname=key.section('@',0,0);
  sname=sname.section(' ',-1,-1);
  sname=sname.section('.',0,0);
  sname=sname.section('(',-1,-1);
  sname.append(".asc");
  sname.prepend(QDir::homeDirPath()+"/");
  u.setPath(sname);

  popupName *dial=new popupName(i18n("Export Public Key To"),this, "export_key", u,true);
  dial->exportAttributes->setChecked(true);

  if (dial->exec()==QDialog::Accepted)
  {
    ////////////////////////// export to file
    QString expname;
    bool exportAttr=dial->exportAttributes->isChecked();
    KProcIO *p=new KProcIO();
    *p<<"gpg"<<"--no-tty";
    if (dial->checkFile->isChecked())
    {
      expname=dial->newFilename->text().stripWhiteSpace();
      if (!expname.isEmpty())
      {
        QFile fgpg(expname);
        if (fgpg.exists())  fgpg.remove();
        *p<<"--output"<<QFile::encodeName(expname)<<"--export"<<"--armor";
        if (!exportAttr) *p<<"--export-options"<<"no-include-attributes";
        *p<<keysList2->currentItem()->text(5);
        p->start(KProcess::Block);
        if (fgpg.exists()) KMessageBox::information(this,i18n("Your public key \"%1\" was successfully exported\n").arg(expname));
        else KMessageBox::sorry(this,i18n("Your public key could not be exported\nCheck the key."));
      }
    }
    else
    {
      message="";
      *p<<"--export"<<"--armor";
      if (!exportAttr) *p<<"--export-options"<<"no-include-attributes";
      *p<<keysList2->currentItem()->text(5);
      if (dial->checkClipboard->isChecked()) QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessExportClip(KProcess *)));
      else QObject::connect(p, SIGNAL(processExited(KProcess *)),this, SLOT(slotProcessExportMail(KProcess *)));
      QObject::connect(p, SIGNAL(readReady(KProcIO *)),this, SLOT(slotReadProcess(KProcIO *)));
      p->start(KProcess::NotifyOnExit,false);
    }
  }
}

void listKeys::slotReadProcess(KProcIO *p)
{
  QString outp;
  while (p->readln(outp)!=-1) message+=outp+"\n";
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
  if (keysList2->currentItem()->depth()!=0)
  {
    if (keysList2->currentItem()->text(0)==i18n("Photo Id"))
    {
      //////////////////////////    display photo
      KProcIO *p=new KProcIO();
      QString popt="kview %i";
      *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<QFile::encodeName(popt)<<"--list-keys"<<keysList2->currentItem()->parent()->text(5);
      p->start(KProcess::DontCare,true);
      return;
    }
    else
      return;
  }

  /////////////   open a key info dialog (KgpgKeyInfo, see begining of this file)
  QString key=keysList2->currentItem()->text(5);
  if (key!="")
  {
    KgpgKeyInfo *opts=new KgpgKeyInfo(0,0,key);

    opts->exec();

    delete opts;
  }
}

void listKeys::signkey()
{
  ///////////////  sign a key
  if (keysList2->currentItem()==NULL)
    return;
  if (keysList2->currentItem()->depth()!=0)
    return;
  bool islocal=false;
  QString keyID,keyMail;
  //////////////////  open a key selection dialog (KgpgSelKey, see begining of this file)
  KgpgSelKey *opts=new KgpgSelKey(this);

  if (opts->exec()==QDialog::Accepted)
  {
    keyID=QString(opts->getkeyID());
    keyMail=QString(opts->getkeyMail());
    islocal=opts->getlocal();
  }
  else
  {
    delete opts;
    return;
  }
  delete opts;
  QString ask=i18n("Are you sure you want to sign key\n%1 with key %2?\n"
                   "You should always check fingerprint before signing!").arg(keysList2->currentItem()->text(0)).arg(keyMail);

  if (KMessageBox::warningYesNo(this,ask)!=KMessageBox::Yes)
    return;
  KgpgInterface *signKeyProcess=new KgpgInterface();
  signKeyProcess->KgpgSignKey(keysList2->currentItem()->text(5),keyID,keyMail,islocal);

  connect(signKeyProcess,SIGNAL(signatureFinished(int)),this,SLOT(signatureResult(int)));
}

void listKeys::signatureResult(int success)
{
  if (success==3)
    refreshkey();
  if (success==2)
    KMessageBox::sorry(this,i18n("Bad passphrase, try again."));
}


void listKeys::importsignkey()
{
  ///////////////  sign a key
  if (keysList2->currentItem()==NULL)
    return;
kServer=new keyServer(0,"server_dialog",false,WDestructiveClose);
kServer->kLEimportid->setText(keysList2->currentItem()->text(5));
//kServer->Buttonimport->setDefault(true);
kServer->slotImport();
//kServer->show();
connect( kServer->importpop, SIGNAL( destroyed() ) , this, SLOT( importfinished()));
//connect( kServer , SIGNAL( destroyed() ) , this, SLOT( refreshkey()));
 }


void listKeys::importfinished()
{
if (kServer) kServer=0L;
 refreshkey();
}


void listKeys::delsignkey()
{
  ///////////////  sign a key
  if (keysList2->currentItem()==NULL)
    return;
  if (keysList2->currentItem()->depth()>1)
  {
    KMessageBox::sorry(this,i18n("Edit key manually to delete this signature."));
    return;
  }

  QString signID,parentKey,signMail,parentMail;

  //////////////////  open a key selection dialog (KgpgSelKey, see begining of this file)
  parentKey=keysList2->currentItem()->parent()->text(5);
  signID=keysList2->currentItem()->text(5);
  parentMail=keysList2->currentItem()->parent()->text(0);
  signMail=keysList2->currentItem()->text(0);

  if (parentKey==signID)
  {
    KMessageBox::sorry(this,i18n("Edit key manually to delete a self-signature."));
    return;
  }
  QString ask=i18n("Are you sure you want to delete signature\n%1 from key %2?").arg(signMail).arg(parentMail);

  if (KMessageBox::warningYesNo(this,ask)!=KMessageBox::Yes)
    return;
  KgpgInterface *delSignKeyProcess=new KgpgInterface();
  delSignKeyProcess->KgpgDelSignature(parentKey,signID);
  connect(delSignKeyProcess,SIGNAL(delsigfinished(bool)),this,SLOT(delsignatureResult(bool)));
}

void listKeys::delsignatureResult(bool success)
{
  if (success)
    refreshkey();
  else
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
  if (genkey->exec()==QDialog::Accepted)
  {
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
        if (code!=QDialog::Accepted) return;
        if (password.length()<5) KMessageBox::sorry(this,i18n("This passphrase is not secure enough.\nMinimum length= 5 characters"));
        else goodpass=true;
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

      if (ktype=="ElGamal") proc->writeStdin("Key-Type: 20");
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
    }

    else  ////// start expert (=konsole) mode
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
      }
      else if (waitpid (pid, &status, 0) != pid)  ////// parent process wait for end of child
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
  int result=KMessageBox::warningYesNo(this,
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
  if (keysList2->secretList.find(keysList2->currentItem()->text(5))!=-1) deleteseckey();
  else
  {
    QString res=keysList2->currentItem()->text(0);
    res.replace(QRegExp("<"),"&lt;");
    int result=KMessageBox::warningYesNo(this,i18n("<p>Delete public key <b>%1</b> ?</p>").arg(res),i18n("Warning"),i18n("Delete"));
    if (result!=KMessageBox::Yes)
      return;
    else
      deletekey();
  }
}

void listKeys::deletekey()
{
  KProcess gp;
  gp << "gpg"
  << "--no-tty"
  << "--no-secmem-warning"
  << "--batch"
  << "--yes"
  << "--delete-key"
  << keysList2->currentItem()->text(5);
  gp.start(KProcess::Block);
  refreshkey();
}


void listKeys::slotPreImportKey()
{
  popupImport *dial=new popupImport(i18n("Import Key From"),this, "import_key");

  if (dial->exec()==QDialog::Accepted)
  {
    bool importSecret = dial->importSecretKeys->isChecked();

    if (dial->checkFile->isChecked())
    {
      QString impname=dial->newFilename->text().stripWhiteSpace();
      if (!impname.isEmpty())
      {
        ////////////////////////// import from file
        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKeyURL(impname, importSecret);
        connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkey()));
      }
    }
    else
    {
      QString keystr = kapp->clipboard()->text();
      if (!keystr.isEmpty())
      {
        KgpgInterface *importKeyProcess=new KgpgInterface();
        importKeyProcess->importKey(keystr, importSecret);
        connect(importKeyProcess,SIGNAL(importfinished()),this,SLOT(refreshkey()));
      }
    }
  }
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
void listKeys::refreshkey()
{
  keysList2->refreshkeylist();
}

void KeyView::refreshkeylist()
{
  ////////   update display of keys in main management window
  FILE *fp;
  photoKeysList="";
  QString tst,cycle,revoked;
  char line[300];
  UpdateViewItem *item=NULL;
  SmallViewItem *itemsub=NULL;
  SmallViewItem *itemuid=NULL;
  SmallViewItem *itemsig=NULL;
  bool noID=false;

  clear();
  cycle="";
  FILE *fp2;
  QString block="idre";
  QString issec="";
  secretList="";
  revoked="";
  fp2 = popen("gpg --no-secmem-warning --no-tty --list-secret-keys", "r");
  while ( fgets( line, sizeof(line), fp2))
    issec+=line;
  pclose(fp2);

  fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-sigs", "r");
  while ( fgets( line, sizeof(line), fp))
  {
    tst=line;

    if (tst.startsWith("uid") || tst.startsWith("uat"))
    {
      gpgKey uidKey=extractKey(tst);

      //          QString tr=trustString(trust).gpgkeytrust;
      if (tst.startsWith("uat"))
      {
        photoKeysList+=item->text(5);
        itemuid= new SmallViewItem(item,i18n("Photo Id"),uidKey.gpgkeytrust,"-","-","-","-");
        itemuid->setPixmap(0,pixuserphoto);
        cycle="uid";
      }
      else
      {
        itemuid= new SmallViewItem(item,extractKeyName(uidKey.gpgkeyname,uidKey.gpgkeymail),uidKey.gpgkeytrust,"-","-","-","-");
        if (noID)
        {
          item->setText(0,extractKeyName(uidKey.gpgkeyname,uidKey.gpgkeymail));
          noID=false;
        }
        itemuid->setPixmap(0,pixuserid);
        cycle="uid";
      }
    }
    else

      if (tst.startsWith("rev"))
      {
        revoked+=QString("0x"+tst.section(':',4,4).right(8)+" ");
      }
      else

        if (tst.startsWith("sig"))
        {
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
        }
        else

          if (tst.startsWith("sub"))
          {
            gpgKey subKey=extractKey(tst);
            tst=i18n("%1 subkey").arg(subKey.gpgkeyalgo);
            itemsub= new SmallViewItem(item,tst,subKey.gpgkeytrust,subKey.gpgkeyexpiration,subKey.gpgkeysize,subKey.gpgkeycreation,subKey.gpgkeyid);
            itemsub->setPixmap(0,pixkeySingle);
            cycle="sub";

          }
          else
            if (tst.startsWith("pub"))
            {
              noID=false;
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
                  if (currentRevoke.find(subcurrent->text(5))!=-1)
                  {
                    subcurrent->setText(1,i18n("Revoked"));
                    found=true;
                  }
                  while (subcurrent->nextSibling())
                  {
                    subcurrent = subcurrent->nextSibling();
                    if (currentRevoke.find(subcurrent->text(5))!=-1)
                    {
                      subcurrent->setText(1,i18n("Revoked"));
                      found=true;
                    }
                  }
                }

                while ( current->nextSibling() )
                {
                  current = current->nextSibling();
                  if (currentRevoke.find(current->text(5))!=-1)
                  {
                    current->setText(1,i18n("Revoked"));
                    found=true;
                  }

                  QListViewItem *subcurrent = current->firstChild();
                  if (subcurrent)
                  {
                    if (currentRevoke.find(subcurrent->text(5))!=-1)
                    {
                      subcurrent->setText(1,i18n("Revoked"));
                      found=true;
                    }
                    while (subcurrent->nextSibling())
                    {
                      subcurrent = subcurrent->nextSibling();
                      if (currentRevoke.find(subcurrent->text(5))!=-1)
                      {
                        subcurrent->setText(1,i18n("Revoked"));
                        found=true;
                      }
                    }
                  }
                }
                if (!found)
                  (void) new SmallViewItem(item,i18n("Revocation Certificate"),"+","+","+","+",currentRevoke);
              }
              gpgKey pubKey=extractKey(tst);

              QString isbold="";
              if (pubKey.gpgkeyid==defKey)
                isbold="on";
              if (pubKey.gpgkeyname.isEmpty()) noID=true;


              //QTextCodec::codecForContent(locallyEncoded,locallyEncoded.length()); // get the codec for KOI8-R

              item=new UpdateViewItem(this,extractKeyName(pubKey.gpgkeyname,pubKey.gpgkeymail),pubKey.gpgkeytrust,pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold);
              //item=new UpdateViewItem(this,codec->toUnicode( locallyEncoded),pubKey.gpgkeytrust,pubKey.gpgkeyexpiration,pubKey.gpgkeysize,pubKey.gpgkeycreation,pubKey.gpgkeyid,isbold);

              cycle="pub";

              if (issec.find(pubKey.gpgkeyid.right(8),0,FALSE)!=-1)
              {
                item->setPixmap(0,pixkeyPair);
                secretList+=pubKey.gpgkeyid;
              }
              else
              {
                item->setPixmap(0,pixkeySingle);
              }
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
      if (currentRevoke.find(subcurrent->text(5))!=-1)
      {
        subcurrent->setText(1,i18n("Revoked"));
        found=true;
      }
      while (subcurrent->nextSibling())
      {
        subcurrent = subcurrent->nextSibling();
        if (currentRevoke.find(subcurrent->text(5))!=-1)
        {
          subcurrent->setText(1,i18n("Revoked"));
          found=true;
        }
      }
    }

    while ( current->nextSibling() )
    {
      current = current->nextSibling();
      if (currentRevoke.find(current->text(5))!=-1)
      {
        current->setText(1,i18n("Revoked"));
        found=true;
      }

      QListViewItem *subcurrent = current->firstChild();
      if (subcurrent)
      {
        if (currentRevoke.find(subcurrent->text(5))!=-1)
        {
          subcurrent->setText(1,i18n("Revoked"));
          found=true;
        }
        while (subcurrent->nextSibling())
        {
          subcurrent = subcurrent->nextSibling();
          if (currentRevoke.find(subcurrent->text(5))!=-1)
          {
            subcurrent->setText(1,i18n("Revoked"));
            found=true;
          }
        }
      }
    }
    if (!found)
      (void) new SmallViewItem(item,i18n("Revocation Certificate"),"+","+","+","+",currentRevoke);
  }
  setSelected(firstChild(),true);
  if (columnWidth(0)>150)
    setColumnWidth(0,150);
}

QString KeyView::extractKeyName(QString name,QString mail)
{
  if (displayMailFirst) return QString(mail+" ("+name+")");
  return QString(name+" ("+mail+")");
}

gpgKey KeyView::extractKey(QString keyColon)
{
  gpgKey ret;

  ret.gpgkeysize=keyColon.section(':',2,2);
  ret.gpgkeycreation=keyColon.section(':',5,5);
  QString tid=keyColon.section(':',4,4);
  ret.gpgkeyid=QString("0x"+tid.right(8));
  ret.gpgkeyexpiration=keyColon.section(':',6,6);
  if (ret.gpgkeyexpiration=="") ret.gpgkeyexpiration=i18n("Unlimited");
  QString fullname=keyColon.section(':',9,9);
  if (fullname.find("<")!=-1)
  {
    ret.gpgkeymail=fullname.section('<',-1,-1);
    ret.gpgkeymail.truncate(ret.gpgkeymail.length()-1);
    ret.gpgkeyname=fullname.section('<',0,0);
    if (ret.gpgkeyname.find("(")!=-1) ret.gpgkeyname=ret.gpgkeyname.section('(',0,0);
  }
  else
  {
    ret.gpgkeymail="";
    ret.gpgkeyname=fullname.section('(',0,0);
  }
  QString algo=keyColon.section(':',3,3);
  if (!algo.isEmpty())
    switch( algo.toInt() )
    {
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
  QString tr;
  switch( trust[0] )
  {
  case 'o':
    tr=i18n("Unknown");
    break;
  case 'i':
    tr=i18n("Invalid");
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
  ret.gpgkeytrust=tr;

  return ret;
}
#include "listkeys.moc"
