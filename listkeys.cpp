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
#include <qpainter.h>
#include <qvbox.h>
#include <qclipboard.h>
#include <qkeysequence.h>

#include <kio/netaccess.h>
#include <kurl.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kshortcut.h>

#include "listkeys.h"
#include "kgpginterface.h"


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
  if ((Itemdefault==true) && (column==0))
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
  resize(400,100);
  QWidget *page=new QWidget(this);
QLabel *labelname = new QLabel(page);
QLabel *labelmail = new QLabel(page);
QLabel *labeltype = new QLabel(page);
QLabel *labellength = new QLabel(page);
QLabel *labelcreation = new QLabel(page);
QLabel *labelexpire = new QLabel(page);
QLabel *labeltrust = new QLabel(page);
QLabel *labelid = new QLabel(page);
QLabel *labelfinger = new QLabel(i18n("Fingerprint :"),page);
  //QString labelname,labelmail,labeltype,labellength,labelfinger,labelcreation,labelexpire,labeltrust,labelid,fingervalue;
  //QVBoxLayout *vbox=new QVBoxLayout(page,2);


  FILE *pass;
  char gpgcmd[200]="",line[200]="";
  QString opt,tid;
  bool isphoto=false;
  strcat(gpgcmd,"gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-key ");
  strcat(gpgcmd,sigkey.latin1());

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
              tr= "Unknown" ;
              break;
          case 'i':
              tr= "Invalid";
              break;
          case 'd':
              tr="Disabled";
              break;
          case 'r':
              tr="Revoked";
              break;
          case 'e':
              tr="Expired";
              break;
          case 'q':
              tr="Undefined";
              break;
          case 'n':
              tr="None";
              break;
          case 'm':
              tr="Marginal";
              break;
          case 'f':
              tr="Full";
              break;
          case 'u':
              tr="Ultimate";
              break;
          default:
              tr="?";
              break;
          }

           tid=opt.section(':',4,4);
          QString id=QString("0x"+tid.right(8));

          QString fullname=opt.section(':',9,9);
          //fullname.replace(fullname.find('<'),1,QString("\""));
          //fullname.replace(fullname.find('>'),1,QString("\""));
          if (opt.section(':',6,6)=="")

            labelexpire->setText(i18n("Expiration: never"));
				else
				labelexpire->setText(i18n("Expiration: ")+opt.section(':',6,6));
				labelcreation->setText(i18n("Creation: ")+opt.section(':',5,5));
				labelname=new QLabel(i18n("Name: ")+fullname.section('<',0,0),page);
				labellength->setText(i18n("Length: ")+opt.section(':',2,2));
				labeltrust->setText(i18n("Trust: ")+tr);
				labelid->setText(i18n("ID: ")+tid);

				labelname->setText(i18n("Name: ")+fullname.section('<',0,0));
				fullname=fullname.section('<',1,1);
				fullname=fullname.section('<',1,1);
				fullname=fullname.section('>',0,0);
				fullname=fullname.section('>',0,0);
				labelmail->setText(i18n("E-Mail: ")+fullname);
				labeltype->setText(i18n("Algorithm: ")+algo);
        }
      if (opt.startsWith("fpr"))
        fingervalue=opt.section(':',9,9);
    }
  pclose(pass);


  QGridLayout *Form1Layout = new QGridLayout( page, 1, 1, 11, 6, "Form1Layout");

  Form1Layout->addWidget( labelid, 7, 1 );

  Form1Layout->addWidget( labeltrust, 6, 1 );

  Form1Layout->addWidget( labelexpire, 5, 1 );

  Form1Layout->addWidget( labelcreation, 4, 1 );

  Form1Layout->addWidget( labellength, 3, 1 );

  Form1Layout->addWidget( labeltype, 2, 1 );

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

  if (isphoto==true)
    {
      kgpginfotmp=new KTempFile();
      QString popt="cp %i "+kgpginfotmp->name();
      KProcIO *p=new KProcIO();
      *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<popt.local8Bit()<<"--list-keys"<<tid;
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
  /*
    QString outp;
    while (p->readln(outp)!=-1)
      {
          //message+=outp;
  		if (outp.startsWith("Filename="))
  		{
  		outp.remove(0,9);
  		outp=outp.stripWhiteSpace();
  		QPixmap pixmap; 
  		pixmap.load(outp);
  		keyinfoPhoto->setPixmap(pixmap);
    }
  		
      }
  	*/
  QPixmap pixmap;
  pixmap.load(kgpginfotmp->name());
  keyinfoPhoto->setPixmap(pixmap);
  kgpginfotmp->unlink();
}


////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////   Secret key selection dialog, used when user wants to sign a key
KgpgSelKey::KgpgSelKey(QWidget *parent, const char *name,bool showlocal):KDialogBase( parent, name, true,i18n("Private key list"),Ok | Cancel)
{
  QString keyname;
  QWidget *page = new QWidget(this);
  QLabel *labeltxt;
  KIconLoader *loader = KGlobal::iconLoader();

  keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);

  setMinimumSize(300,100);
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
      local = new QCheckBox(QString(i18n("Local signature (cannot be exported)")),page);
      vbox->addWidget(local);
    }

  FILE *fp;
  QString tst;
  char line[130];

  fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
  while ( fgets( line, sizeof(line), fp))
  {
      tst=line;
      if (tst.find("sec",0,FALSE)!=-1)
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
          if (!tst.isEmpty())
          {
              keyname=tst.section('<',1,1);
              keyname=keyname.section('>',0,0);
              keyname+=" ("+tst.section('<',0,0)+")";
              KListViewItem *item=new KListViewItem(keysListpr,keyname);
              KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, validity: %3").arg(id).arg(tr).arg(val));
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
	userid.stripWhiteSpace();
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
  username=username.section(' ',0,0);
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

///////////////   main window for key management
listKeys::listKeys(QWidget *parent, const char *name, WFlags f) : KMainWindow(parent, name, f)//QDialog(parent,name,TRUE)//KMainWindow(parent, name)
{
  config=kapp->config();
  readOptions();
  showPhoto=false;
  //if (enctodef==true) defKey=defaultKey;
  //else defKey="";
  setCaption(i18n("Key Management"));
  KAction *exportPublicKey = new KAction(i18n("E&xport public key"), "kgpg_export", 0,this, SLOT(slotexport()),actionCollection(),"key_export");
  KAction *deleteKey = new KAction(i18n("&Delete key"),"editdelete", 0,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
  KAction *signKey = new KAction(i18n("&Sign key"), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
  KAction *delSignKey = new KAction(i18n("Delete sign&ature"),0, 0,this, SLOT(delsignkey()),actionCollection(),"key_delsign");
  KAction *infoKey = new KAction(i18n("&Key info"), "kgpg_info", 0,this, SLOT(listsigns()),actionCollection(),"key_info");
  KAction *importKey = new KAction(i18n("&Import key"), "kgpg_import", 0,this, SLOT(slotImportKey()),actionCollection(),"key_import");
  KAction *setDefaultKey = new KAction(i18n("Set as De&fault key"),0, 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
  
  KStdAction::quit(this, SLOT(annule()), actionCollection());
  (void) new KAction(i18n("&Refresh list"), "reload", 0,this, SLOT(refreshkey()),actionCollection(),"key_refresh");
  KAction *editKey = new KAction(i18n("&Edit Key"), "kgpg_edit", 0,this, SLOT(slotedit()),actionCollection(),"key_edit");
  KAction *exportSecretKey = new KAction(i18n("Export secret key"), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
  KAction *deleteKeyPair = new KAction(i18n("Delete key pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
  KAction *generateKey = new KAction(i18n("&Generate key pair"), "kgpg_gen", 0,this, SLOT(slotgenkey()),actionCollection(),"key_gener");
  (void) new KToggleAction(i18n("&Show photos"), "imagegallery", 0,this, SLOT(hidePhoto()),actionCollection(),"key_showp");

  KStdAction::preferences(this, SLOT(slotParentOptions()), actionCollection());
  KStdAction::showToolbar(this, SLOT(showToolBar()), actionCollection());


  KIconLoader *loader = KGlobal::iconLoader();

  pixkeyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
  pixkeySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
  pixsignature=loader->loadIcon("signature",KIcon::Small,20);
  pixuserid=loader->loadIcon("kgpg_identity",KIcon::Small,20);
  pixuserphoto=loader->loadIcon("kgpg_photo",KIcon::Small,20);

  QWidget *page=new QWidget(this);
  QVBoxLayout *vbox=new QVBoxLayout(page,3);
  keysList2 = new KListView(page);
  keysList2->setRootIsDecorated(true);
  keysList2->addColumn( i18n( "Key" ) );
  keysList2->addColumn( i18n( "Trust" ) );
  keysList2->addColumn( i18n( "Validity" ) );
  keysList2->addColumn( i18n( "Size" ) );
  keysList2->addColumn( i18n( "Creation" ) );
  keysList2->addColumn( i18n( "ID" ) );
  keysList2->setShowSortIndicator(true);
  keysList2->setAllColumnsShowFocus(true);
  keysList2->setFullWidth(true);

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
    
	keyPhoto=new QLabel(page);
    keyPhoto->setText("Photo");
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

  //QObject::connect(keysList2,SIGNAL(selectionChanged(QListViewItem *)),this,SLOT(slotstatus(QListViewItem *)));

  ///////////////    get all keys data
  refreshkey();
  createGUI("listkeys.rc");
  keyPhoto->hide();
}

listKeys::~listKeys()
{}

void listKeys::showToolBar()
{
  if( toolBar()->isVisible () )
    toolBar()->hide();
  else
    toolBar()->show();
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
  if (showPhoto==false)
    return;
  if (keysList2->currentItem()->depth()!=0)
    {
      keyPhoto->setText(i18n("No\nphoto"));
      return;
    }
  QString CurrentID=keysList2->currentItem()->text(5);
  if (photoKeysList.find(CurrentID)!=-1)
    {
      kgpgtmp=new KTempFile();
      QString popt="cp %i "+kgpgtmp->name();
      KProcIO *p=new KProcIO();
      *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<popt.local8Bit()<<"--list-keys"<<CurrentID;
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
  QString defaultkey=config->readEntry("default key");
  if (encrypttodefault==true)
    defKey=defaultkey;
  else
    defKey="";
}



void listKeys::slotParentOptions()
{
  kgpgOptions *opts=new kgpgOptions(this,0);
  opts->exec();
  delete opts;
  readOptions();
  /*

    KgpgApp *win=(KgpgApp *) parent();
    win->slotOptions();
    if (win->encrypttodefault==true) defKey=win->defaultkey;
    else defKey="";
  */
  refreshkey();
}

//////////////////////////////////////////////////////////////////  clipboard test

/*
{


 int main(int argc, char** argv)
 {
 QApplication app(argc,argv);

 QClipboard* clip = QApplication::clipboard();
 clip->setSelectionMode(true);
 QMimeSource* data = clip ->data();
 if (!data)
 qFatal("No data in clipboad!");
 int pos = 0;

 while (data->format(pos))
 {
 qDebug("Format supported:%s", data->format(pos));
 pos++;
 }
 }
}
*/

void listKeys::slotSetDefKey()
{
  QString block="Invalid, Disabled, Revoked, Expired,?";
  QString key=keysList2->currentItem()->text(5);

  if  (defKey=="")
    {
      KMessageBox::sorry(0,i18n("Before setting a default key, you must enable default key encryption in the Options dialog"));
      return;
    }

  if  (block.find(keysList2->currentItem()->text(1))!=-1)
    {
      KMessageBox::sorry(0,i18n("Sorry, this key is not valid for encryption or not trusted..."));
      return;
    }
  /////////////// revert old default key to normal icon
  defKey=keysList2->currentItem()->text(5);
  refreshkey();

  QListViewItem *olddef = keysList2->firstChild();
  while (olddef->text(5)!=defKey)
    if (olddef->nextSibling())
      olddef = olddef->nextSibling();
    else
      break;
  keysList2->setSelected(olddef,true);

  ////////////// store new default key

  config->setGroup("General Options");
  config->writeEntry("default key",defKey);

  /*
    KgpgApp *win=(KgpgApp *) parent();
    win->defaultkey=defKey;

    win->view->pubdefaultkey=defKey;
    win->saveOptions();*/
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
          if (secretList.find(sel->text(5))!=-1)
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
  char gpgcmd[1024] = "\0";
  QString warn=i18n("Secret keys SHOULD NOT be saved  in an unsafe place.\n"
                    "If someone else can access this file, encryption with this key will be compromised !\nContinue key export ?");
  int result=KMessageBox::warningYesNo(this,warn,i18n("Warning"));
  if (result!=KMessageBox::Yes)
    return;

  QString key=keysList2->currentItem()->text(0);
  KURL *exp=new KURL(QString(QDir::currentDirPath()+"/"+key.section("@",0,0)+".asc"));
  KURL url=KFileDialog::getSaveURL(exp->path(),"*.asc|*.asc files", this, i18n("Export PRIVATE KEY as..."));

  if(!url.isEmpty())
    {
      FILE *fp;
      strcat(gpgcmd,"gpg --no-tty --armor --export-secret-keys ");
      strcat(gpgcmd,QString(keysList2->currentItem()->text(5)+" > "+url.path()).latin1());
      QFile fgpg(url.path());
      if (fgpg.exists())
        fgpg.remove();

      fp = popen(gpgcmd, "r");
      pclose(fp);

      if (fgpg.exists())
        {
          QString mess=i18n("Your PRIVATE key \"%1\"  was successfully exported\nDO NOT leave it in an insecure place !").arg(url.filename());
          KMessageBox::information(0,mess);
        }
      else
        KMessageBox::sorry(0,i18n("Your secret key could not be exported\nCheck the key..."));
    }

}


void listKeys::slotexport()
{
  /////////////////////  export key
  if (keysList2->currentItem()==NULL)
    return;
  if (keysList2->currentItem()->depth()!=0)
    return;

  char gpgcmd[1024] = "\0",line[130]="";
  exportresult="";

  QString key=keysList2->currentItem()->text(0);

  QString sname=key.section('@',0,0);
  sname=sname.section('.',0,0);
  sname.append(".asc");
  sname.prepend(QDir::homeDirPath()+"/");

  KURL u;
  u.setPath(sname);
  popupName *dial=new popupName(i18n("Export public key to"),this, "export_key", u,true);

  //popupName *dial=new popupName(this,i18n("Export public key to"),sname,true);
  /////////////   open export dialog (KgpgExport, see begining of this file)
  dial->exec();

  if (dial->result()==true)
    {
      ////////////////////////// export to file
      FILE *fp;
      QString expname;
      expname="";
      if (dial->getfmode()==true)
        expname=dial->getfname();
      bool attr=dial->exportAttributes->isChecked();
      //if (attr==true) KMessageBox::sorry(0,"rtz");
      //    if (dial->getmailmode()==true) expname=sname;

      strcat(gpgcmd,"gpg --no-tty --export --armor ");
      if (attr==true)
        strcat(gpgcmd,"--export-options include-attributes ");
      strcat(gpgcmd,keysList2->currentItem()->text(5).latin1());
      QFile fgpg(expname);
      if (expname!="")
        {
          strcat(gpgcmd,QString(" > ")+expname.latin1());
          if (fgpg.exists())
            fgpg.remove();
        }

      QString tst="";
      fp=popen(gpgcmd,"r");
      while ( fgets( line, sizeof(line), fp))    /// read output
        tst+=line;
      pclose(fp);

      if (dial->getmailmode()==true)
        {
          ///////////////////////// send key by mail
          KProcIO *proc=new KProcIO();
          QString subj=QString("Public key for %1").arg(key);
          *proc<<"kmail"<<"--subject"<<subj<<"--body"<<tst;
          //QObject::connect(proc, SIGNAL(processExited(KProcess *)),this, SLOT(removetemp(KProcess *)));
          //proc->start(KProcess::NotifyOnExit);
          proc->start(KProcess::DontCare);
          return;
        }

      ////

      if  ((expname=="") && (tst!=""))
        {
          // Copy text to the clipboard (paste)

          QClipboard *cb = QApplication::clipboard();
          cb->setText(tst);
          ////////////////////////////////  copy key to clipboard
          //KgpgApp *win=(KgpgApp *) parent();
          //win->view->editor->setText(tst);
        }

      /////

      if (expname!="")
        {
          if (fgpg.exists())
            {
              QString mess=i18n("Your public key \"%1\" was successfully exported\n").arg(dial->getfname());
              KMessageBox::information(0,mess);
            }
          else
            KMessageBox::sorry(0,i18n("Your public key could not be exported\nCheck the key..."));
        }
    }
  else
    {
      delete dial;
      return;
    }
  delete dial;

}


void listKeys::listsigns()
{
  if (keysList2->currentItem()==NULL)
    return;
  if (keysList2->currentItem()->depth()!=0)
    {
      if (keysList2->currentItem()->text(0)==i18n("Photo ID"))
        {
          //////////////////////////    display photo
          KProcIO *p=new KProcIO();
          QString popt="kview %i";
          *p<<"gpg"<<"--show-photos"<<"--photo-viewer"<<popt.local8Bit()<<"--list-keys"<<keysList2->currentItem()->parent()->text(5);
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

  opts->exec();
  if (opts->result()==true)
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
  QString ask=i18n("Are you sure you want to sign key\n%1 with key %2 ?\n"
                   "You should always check fingerprint before signing !").arg(keysList2->currentItem()->text(0)).arg(keyMail);

  if (KMessageBox::warningYesNo(this,ask)!=KMessageBox::Yes)
    return;
  KgpgInterface *signKeyProcess=new KgpgInterface();
  signKeyProcess->KgpgSignKey(keysList2->currentItem()->text(5),keyID,keyMail,islocal);

  connect(signKeyProcess,SIGNAL(signatureFinished(int)),this,SLOT(signatureResult(int)));
}

void listKeys::signatureResult(int success)
{
  if (success==0)
    refreshkey();
  if (success==2)
    KMessageBox::sorry(this,i18n("Bad passphrase, try again"));
  if (success==1)
    KMessageBox::sorry(this,i18n("Requested operation was unsuccessful, please edit the key manually"));
}



void listKeys::delsignkey()
{
  ///////////////  sign a key
  if (keysList2->currentItem()==NULL)
    return;
  if (keysList2->currentItem()->depth()>1)
    {
      KMessageBox::sorry(this,i18n("Edit key manually to delete this signature"));
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
      KMessageBox::sorry(this,i18n("Edit key manually to delete a self-signature"));
      return;
    }
  QString ask=i18n("Are you sure you want to delete signature\n%1 from key %2 ?").arg(signMail).arg(parentMail);

  if (KMessageBox::warningYesNo(this,ask)!=KMessageBox::Yes)
    return;
  KgpgInterface *delSignKeyProcess=new KgpgInterface();
  delSignKeyProcess->KgpgDelSignature(parentKey,signID);
  connect(delSignKeyProcess,SIGNAL(encryptionfinished(bool)),this,SLOT(delsignatureResult(bool)));
}

void listKeys::delsignatureResult(bool success)
{
  if (success==true)
    refreshkey();
  else
    KMessageBox::sorry(this,i18n("Requested operation was unsuccessful, please edit the key manually"));
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
	<<keysList2->currentItem()->text(5).latin1()
	<<"help";
  kp.start(KProcess::Block);
  refreshkey();
}

void listKeys::slotprocresult(KProcess *)
{
    KIO::NetAccess::removeTempFile(tempKeyFile);
    KMessageBox::information(0,message);
    refreshkey();
}

void listKeys::slotprocread(KProcIO *p)
{
  QString outp;
  while (p->readln(outp)!=-1)
    {
      if (outp.find("http-proxy")==-1)
        message+=outp+"\n";
    }
}

void listKeys::slotgenkey()
{
  QString filecont;
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
          int code=KPasswordDialog::getNewPassword(password,QString(i18n("Enter passphrase for %1:").arg(kmail)));
          if (code==QDialog::Accepted)
            {
              pop = new QDialog( this,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(pop,3);
              QLabel *tex=new QLabel(pop);
              tex->setText(i18n("Generating new key pair... please wait ..."));
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

              filecont+=QString("Key-Length:%1\n").arg(ksize);
              proc->writeStdin(QString("Name-Real:%1").arg(kname));
              proc->writeStdin(QString("Name-Email:%1").arg(kmail));
              if (kcomment!="")
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
                proc->writeStdin(QString("Expire-Date:%y").arg(knumb));
              proc->writeStdin("%commit");
              proc->writeStdin("EOF");
            }
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

    int result=KMessageBox::warningYesNo(this,
                                         i18n("Delete SECRET KEY pair %1 ?\nDeleting this key pair means you will never be able to decrypt files encrypted with this key anymore!!!").arg(res),
                                         i18n("Warning"),
                                         i18n("Delete"));
    if (result!=KMessageBox::Yes)
        return;

    KProcess *conprocess=new KProcess();
    *conprocess<< "konsole"<<"-e"<<"gpg";
    *conprocess<<"--no-secmem-warning"<<"--delete-secret-key"<<keysList2->currentItem()->text(5);
    QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(deletekey()));
    conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::confirmdeletekey()
{
  QString res=keysList2->currentItem()->text(0);

  int result=KMessageBox::warningYesNo(this,i18n("Delete public key %1 ?").arg(res),i18n("Warning"),i18n("Delete"));
  if (result!=KMessageBox::Yes)
    return;
  else
    deletekey();

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

void listKeys::slotImportKey()
{
  /////////////      import a key
  KURL url=KFileDialog::getOpenURL(QString::null,i18n("*.asc|*.asc files"), this,i18n("Select key file to import"));
  if (url.isEmpty())
      return;

  if( KIO::NetAccess::download( url, tempKeyFile ) )
  {
      message=QString::null;
      KProcIO *conprocess=new KProcIO();
      *conprocess<< "gpg";
      *conprocess<<"--no-tty"<<"--no-secmem-warning"<<"--allow-secret-key-import"<<"--import"<<tempKeyFile;
      QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresult(KProcess *)));
      QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
      conprocess->start(KProcess::NotifyOnExit,true);
    }
}



void listKeys::refreshkey()
{
  ////////   update display of keys in main management window
  FILE *fp;
  photoKeysList="";
  QString tst,cycle,keyname,revoked;
  char line[300];
  UpdateViewItem *item=NULL;
  SmallViewItem *itemsub=NULL;
  SmallViewItem *itemuid=NULL;
  SmallViewItem *itemsig=NULL;
  keysList2->clear();
  cycle="";
  char gpgcmd2[1024] = "\0";
  FILE *fp2;
  QString block="idre";
  issec="";
  secretList="";
  revoked="";
  strcat(gpgcmd2,"gpg --no-secmem-warning --no-tty --list-secret-keys ");
  //strcat(gpgcmd2,tst);
  fp2 = popen(gpgcmd2, "r");
  while ( fgets( line, sizeof(line), fp2))
    issec+=line;
  pclose(fp2);

  fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-sigs", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;

      if (tst.startsWith("uid") || tst.startsWith("uat"))
        {
          
          const QString trust=tst.section(':',1,1);

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
          if (tst.startsWith("uat"))
            {
              photoKeysList+=item->text(5);
              itemuid= new SmallViewItem(item,i18n("Photo ID"),tr,"-","-","-","-");
              itemuid->setPixmap(0,pixuserphoto);
              cycle="uid";
            }

          else
            if (tst.section(':',9,9).find("<",0,FALSE)!=-1)
              {
                QString uidname=tst.section(':',9,9).section('<',1,1);
                  uidname=uidname.section('>',0,0);
                  uidname+=" ("+tst.section(':',9,9).section('<',0,0)+")";

                itemuid= new SmallViewItem(item,uidname,tr,"-","-","-","-");
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
              QString fsigname=tst.section(':',9,9);
              QString islocalsig=tst.section(':',10,10);
              const QString creat=tst.section(':',5,5);
              const QString tid=tst.section(':',4,4);
              QString id=QString("0x"+tid.right(8));
              QString val=tst.section(':',6,6);
              if (val=="")
                val=i18n("Unlimited");
              if (fsigname.find("<",0,FALSE)!=-1)
                {
                  //tst=signame.section('<',1,1);
                  //signame=signame.section('<',0,0);
                  //tst=tst.section('>',0,0);

                  QString signame=fsigname.section('<',1,1);
                  signame=signame.section('>',0,0);
                  signame+=" ("+fsigname.section('<',0,0)+")";

                  if (islocalsig.endsWith("l"))
                    signame+=i18n(" [local]");

                  if (cycle=="pub")
                    itemsig= new SmallViewItem(item,signame,"-",val,"-",creat,id);
                  if (cycle=="sub")
                    itemsig= new SmallViewItem(itemsub,signame,"-",val,"-",creat,id);
                  if (cycle=="uid")
                    itemsig= new SmallViewItem(itemuid,signame,"-",val,"-",creat,id);

                  itemsig->setPixmap(0,pixsignature);
                  //KMessageBox::sorry(0,cycle+"::"+signame);
                  //              (void) new KListViewItem(keysListsig,signame,opt,islocalsig,date,id);
                }
            }
          else

            if (tst.startsWith("sub"))
              {
                QString algo=tst.section(':',3,3);
                const QString creat=tst.section(':',5,5);
                const QString tid=tst.section(':',4,4);
                QString id=QString("0x"+tid.right(8));
                const QString trust=tst.section(':',1,1);
                QString val=tst.section(':',6,6);
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
                QString klength=tst.section(':',2,2);
                tst=i18n("%1 subkey").arg(algo);
                //      pos=tst.find("<",0,FALSE);
                if (!tst.isEmpty())
                  {
                    itemsub= new SmallViewItem(item,tst,tr,val,klength,creat,id);
                    itemsub->setPixmap(0,pixkeySingle);
                    cycle="sub";
                  }
              }
            else
              if (tst.startsWith("pub"))
                {
                  while (revoked!="")   ///////////////   there are revoked sigs in previous key
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
                      if (found==false)
                        (void) new SmallViewItem(item,i18n("Revocation certificate"),"+","+","+","+",currentRevoke);
                    }

                  const QString size=tst.section(':',2,2);
                  const QString creat=tst.section(':',5,5);
                  const QString tid=tst.section(':',4,4);
                  QString id=QString("0x"+tid.right(8));
                  const QString trust=tst.section(':',1,1);
                  QString val=tst.section(':',6,6);
                  if (val=="")
                    val="Unlimited";
                  QString tr;
                  switch( trust[0] )
                    {
                    case 'o':
                      tr= "Unknown" ;
                      break;
                    case 'i':
                      tr= "Invalid";
                      break;
                    case 'd':
                      tr="Disabled";
                      break;
                    case 'r':
                      tr="Revoked";
                      break;
                    case 'e':
                      tr="Expired";
                      break;
                    case 'q':
                      tr="Undefined";
                      break;
                    case 'n':
                      tr="None";
                      break;
                    case 'm':
                      tr="Marginal";
                      break;
                    case 'f':
                      tr="Full";
                      break;
                    case 'u':
                      tr="Ultimate";
                      break;
                    default:
                      tr="?";
                      break;
                    }
                  tst=tst.section(':',9,9);
                  //      pos=tst.find("<",0,FALSE);
                  if (tst!="")
                    {
                      keyname=tst.section('<',1,1);
                      keyname=keyname.section('>',0,0);
                      keyname+=" ("+tst.section('<',0,0)+")";
                      //tst=tst.section('>',0,0);
                      QString isbold="";
                      if (id==defKey)
                        isbold="on";
                      item=new UpdateViewItem(keysList2,keyname,tr,val,size,creat,id,isbold);
                      
                      cycle="pub";

                      if (issec.find(id.right(8),0,FALSE)!=-1)
                        {
                          item->setPixmap(0,pixkeyPair);
                          secretList+=id;
                        }
                      else
                        {
                          item->setPixmap(0,pixkeySingle);
                        }
                    }
                }
    }
  pclose(fp);

  while (revoked!="")   ///////////////   there are revoked sigs in previous key
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
      if (found==false)
        (void) new SmallViewItem(item,i18n("Revocation certificate"),"+","+","+","+",currentRevoke);
    }
  keysList2->setSelected(keysList2->firstChild(),true);
  if (keysList2->columnWidth(0)>150)
    keysList2->setColumnWidth(0,150);
}
//#include "listkeys.moc"

