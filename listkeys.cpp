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
#include <qpainter.h>
#include <qvbox.h>
#include <qkeysequence.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kshortcut.h>

#include "listkeys.h"

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
  if (defaultkey=="on") Itemdefault=true;
  else Itemdefault=false;
  setText(0,tst);
  setText(1,tr);
  setText(2,val);
  setText(3,size);
  setText(4,creat);
  setText(5,id);
}


void UpdateViewItem::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
  if ((column==0) && (Itemdefault==true))
    {
      QFont font(p->font());
      font.setBold(true);
      p->setFont(font);
    }
  KListViewItem::paintCell(p, cg, column, width, alignment);
}



////////  window for the key info dialog
KgpgKeyInfo::KgpgKeyInfo(QWidget *parent, const char *name,QString sigkey):KDialogBase( parent, name, true,sigkey,Close)
{
  KListView *keysListsig;
  QString message;
  QLabel *labeltxt,*label2,*label3,*sublabel;
  KLineEdit *finger;

  QWidget *page=new QWidget(this);
  QVBoxLayout *vbox=new QVBoxLayout(page,3);

  //label2=new QLabel(i18n("<b>Key id</b>:"),page);
  sublabel=new QLabel(i18n("<b>Subkey</b>: none"),page);
  label3=new QLabel(i18n("Fingerprint :"),page);

  finger=new KLineEdit("",page);
  finger->setReadOnly(true);
  finger->setPaletteBackgroundColor(QColor(white));
  labeltxt=new QLabel(i18n("Signatures :"),page);
  keysListsig = new KListView( page );
  keysListsig->setRootIsDecorated(false);
  keysListsig->addColumn( i18n( "Name" ) );
  keysListsig->addColumn( i18n( "Email" ) );
  keysListsig->addColumn( i18n( "Local" ) );
  keysListsig->addColumn( i18n( "Date" ) );
  keysListsig->addColumn( i18n( "ID" ) );
  keysListsig->setShowSortIndicator(false);
  keysListsig->setFullWidth(true);


  FILE *pass;
  char gpgcmd[200]="",line[200]="";
  QString opt;

  strcat(gpgcmd,"gpg --no-tty --no-secmem-warning --with-colon --with-fingerprint --list-sigs ");
  strcat(gpgcmd,sigkey);

  pass=popen(gpgcmd,"r");
  while ( fgets( line, sizeof(line), pass))
    {
      opt=line;
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
          QString fullname=opt.section(':',9,9);
          fullname.replace(fullname.find('<'),1,QString("\""));
          fullname.replace(fullname.find('>'),1,QString("\""));
          label2=new QLabel(i18n("<b>%1 key</b>: %2").arg(algo).arg(fullname),page);
          //label2->setText(i18n("<b>%1 key</b>: %2").arg(algo).arg(fullname));
        }


      if (opt.startsWith("fpr"))  finger->setText(opt.section(':',9,9));

      if (opt.startsWith("sig"))
        {
          const QString date=opt.section(':',5,5);
          const QString id=opt.section(':',4,4);
          QString signame=opt.section(':',9,9);
          QString islocalsig=opt.section(':',10,10);
          if (opt.find("<",0,FALSE)!=-1)
            {
              opt=signame.section('<',1,1);
              signame=signame.section('<',0,0);
              opt=opt.section('>',0,0);
              if (islocalsig.endsWith("l")) islocalsig=i18n("yes");
              else islocalsig=i18n("no");
              (void) new KListViewItem(keysListsig,signame,opt,islocalsig,date,id);
            }
        }

      if (opt.startsWith("sub"))
        {
          const QString date=opt.section(':',5,5);
          const QString id=opt.section(':',4,4);
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
          QString klength=opt.section(':',2,2);
          sublabel->setText(i18n("<b>%1 subkey</b>: size: %2, ID: %3").arg(algo).arg(klength).arg(id));
        }
    }

  pclose(pass);


  vbox->addWidget(label2);
  vbox->addWidget(sublabel);
  vbox->addWidget(label3);
  vbox->addWidget(finger);
  vbox->addWidget(labeltxt);
  vbox->addWidget(keysListsig);
  setMainWidget(page);
  page->show();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////   Secret key selection dialog, used when user wants to sign a key
KgpgSelKey::KgpgSelKey(QWidget *parent, const char *name,bool showlocal):KDialogBase( parent, name, true,i18n("Private key list"),Ok | Cancel)
{
  QWidget *page = new QWidget(this);
  QLabel *labeltxt;
  KIconLoader *loader = KGlobal::iconLoader();

  keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);

  setMinimumSize(300,100);
  keysListpr = new KListView( page );
  keysListpr->setRootIsDecorated(false);
  keysListpr->addColumn( i18n( "Name" ) );
  keysListpr->addColumn( i18n( "Trust" ) );
  keysListpr->addColumn( i18n( "Validity" ) );
  keysListpr->setShowSortIndicator(false);
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
          if (val=="") val="Unlimited";
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

          tst=tst.section("<",1,1);
          tst=tst.section(">",0,0);
          if (tst!="")
            {
              KListViewItem *item=new KListViewItem(keysListpr,tst,tr,val);
              item->setPixmap(0,keyPair);
            }
        }
    }
  pclose(fp);

  QObject::connect(keysListpr,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(slotOk()));

  keysListpr->setSelected(keysListpr->firstChild(),true);

  page->show();
  resize(this->minimumSize());
  setMainWidget(page);
}

void KgpgSelKey::slotOk()
{
  if (keysListpr->currentItem()==NULL) reject();
  else accept();
}


QString KgpgSelKey::getkey()
{
  /////  emit selected key
  return(QString(keysListpr->currentItem()->text(0)));
}

bool KgpgSelKey::getlocal()
{
  /////  emit exportation choice
  return(local->isChecked());
}



/////////////////////////////////////////////////////////////////////////////////////////////

///////////////   main window for key management
listKeys::listKeys(QWidget *parent, const char *name,bool enctodef,QString defaultKey):KMainWindow(parent, name)//QDialog(parent,name,TRUE)//KMainWindow(parent, name)
{
  QLabel *labeltxt;
  if (enctodef==true) defKey=defaultKey;
  else defKey="";
  setCaption(name);
//KStdAction::save(this, SLOT(slotFileSave()), actionCollection());
  KAction *exportPublicKey = new KAction(i18n("E&xport public key"), "kgpg_export", 0,this, SLOT(slotexport()),actionCollection(),"key_export");
  KAction *deleteKey = new KAction(i18n("&Delete key"),"editdelete", 0,this, SLOT(confirmdeletekey()),actionCollection(),"key_delete");
  KAction *signKey = new KAction(i18n("&Sign key"), "kgpg_sign", 0,this, SLOT(signkey()),actionCollection(),"key_sign");
  KAction *infoKey = new KAction(i18n("&Key info"), "kgpg_info", 0,this, SLOT(listsigns()),actionCollection(),"key_info");
  KAction *importKey = new KAction(i18n("&Import key"), "kgpg_import", 0,this, SLOT(slotImportKey()),actionCollection(),"key_import");
  KAction *setDefaultKey = new KAction(i18n("Set as De&fault key"),"kgpg_dkey1", 0,this, SLOT(slotSetDefKey()),actionCollection(),"key_default");
  KAction *close = new KAction(i18n("&Close window"), "exit", 0,this, SLOT(annule()),actionCollection(),"key_exit");
  KAction *editKey = new KAction(i18n("&Edit Key"), "kgpg_edit", 0,this, SLOT(slotedit()),actionCollection(),"key_edit");
  KAction *exportSecretKey = new KAction(i18n("Export secret key"), 0, 0,this, SLOT(slotexportsec()),actionCollection(),"key_sexport");
  KAction *deleteKeyPair = new KAction(i18n("Delete key pair"), 0, 0,this, SLOT(deleteseckey()),actionCollection(),"key_pdelete");
  KAction *generateKey = new KAction(i18n("&Generate key pair"), "kgpg_gen", 0,this, SLOT(slotgenkey()),actionCollection(),"key_gener");
  KAction *configure = new KAction(i18n("Default &options"), "configure", 0,this, SLOT(slotParentOptions()),actionCollection(),"key_configure");
  
  
  
  
  KIconLoader *loader = KGlobal::iconLoader();

  dkeyPair=loader->loadIcon("kgpg_dkey2",KIcon::Small,20);
  dkeySingle=loader->loadIcon("kgpg_dkey1",KIcon::Small,20);
  keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
  keySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
  
 QWidget *page=new QWidget(this);
  QVBoxLayout *vbox=new QVBoxLayout(page,3);
  keysList2 = new KListView(page);
  keysList2->setRootIsDecorated(false);
  keysList2->addColumn( i18n( "E-Mail" ) );
  keysList2->addColumn( i18n( "Trust" ) );
  keysList2->addColumn( i18n( "Validity" ) );
  keysList2->addColumn( i18n( "Size" ) );
  keysList2->addColumn( i18n( "Creation" ) );
  keysList2->addColumn( i18n( "ID" ) );
  keysList2->setShowSortIndicator(false);
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
  
  /*
  toolbar=new KToolBar(this);
  vbox->addWidget(toolbar);
  
  exportPublicKey->plug(toolbar);
  signKey->plug(toolbar);
  infoKey->plug(toolbar);
  editKey->plug(toolbar);
  toolbar->insertLineSeparator();
  generateKey->plug(toolbar);
  importKey->plug(toolbar);
  toolbar->insertLineSeparator();
  configure->plug(toolbar);
  close->plug(toolbar);
  
    
  //toolbar->setIconText(KToolBar::IconTextBottom);
  toolbar->enableMoving(false);
*/
  statusbar=new KStatusBar(page);

  //vbox->addWidget(labeltxt);
  vbox->addWidget(keysList2);
  vbox->addWidget(statusbar);
  setCentralWidget(page);
  
  QObject::connect(keysList2,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(listsigns()));
  QObject::connect(keysList2,SIGNAL(contextMenuRequested(QListViewItem *,const QPoint &,int)),
                   this,SLOT(slotmenu(QListViewItem *,const QPoint &,int)));

  QObject::connect(keysList2,SIGNAL(selectionChanged(QListViewItem *)),
                   this,SLOT(slotstatus(QListViewItem *)));

  ///////////////    get all keys data
refreshkey();
createGUI("listkeys.rc");
KMenuBar *menu=KMainWindow::menuBar();
menu->hide();
}

listKeys::~listKeys()
{}

void listKeys::annule()
{
  /////////  cancel & close window
  //exit(0);
close();
  //reject();
}

void listKeys::slotParentOptions()
{
  KgpgApp *win=(KgpgApp *) parent();
  win->slotOptions();
  if (win->encrypttodefault==true) defKey=win->defaultkey;
  else defKey="";
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
  while (olddef->text(5)!=defKey) if (olddef->nextSibling()) olddef = olddef->nextSibling();
    else break;
  keysList2->setSelected(olddef,true);

  ////////////// store new default key

  KgpgApp *win=(KgpgApp *) parent();
  win->defaultkey=defKey;

  win->view->pubdefaultkey=defKey;
  win->saveOptions();
}

void listKeys::slotstatus(QListViewItem *sel)
{
  ////////////  echo key email in statusbar
  QStringList result=keynames.grep(sel->text(5));
  QString res=result.join("");
  res=res.section(',',1,1);
  statusbar->message(res);
}

void listKeys::slotmenu(QListViewItem *sel, const QPoint &pos, int column)
{
  ////////////  popup a different menu depending on which key is selected
  if (sel!=NULL)
    {
      keysList2->setSelected(sel,TRUE);
      if (secretList.find(sel->text(0))!=-1) popupsec->exec(pos);
      else popup->exec(pos);
    }
  else popupout->exec(pos);
}


void listKeys::slotexportsec()
{
  //////////////////////   export secret key
  char gpgcmd[1024] = "\0";
  QString warn=i18n("Secret keys SHOULD NOT be saved  in an unsafe place.\n"
                    "If someone else can access this file, encryption with this key will be compromised !!!");
  int result=KMessageBox::warningContinueCancel(this,warn,i18n("Warning"),i18n("Continue"));
  if (result==KMessageBox::Cancel) return;

  QString key=keysList2->currentItem()->text(0);
  KURL *exp=new KURL(QString(QDir::currentDirPath()+"/"+key.section("@",0,0)+".asc"));
  KURL url=KFileDialog::getSaveURL(exp->path(),"*.asc|*.asc files", this, i18n("Export PRIVATE KEY as..."));

  if(!url.isEmpty())
    {
      FILE *fp;
      strcat(gpgcmd,"gpg --no-tty --armor --export-secret-keys ");
      strcat(gpgcmd,QString(key+" > "+url.path()));
      QFile fgpg(url.path());
      if (fgpg.exists()) fgpg.remove();

      fp = popen(gpgcmd, "r");
      pclose(fp);

      if (fgpg.exists())
        {
          QString mess=i18n("Your PRIVATE key \"%1\"  was successfully exported\nDO NOT leave it in an insecure place !").arg(url.filename());
          KMessageBox::information(0,mess);
        }
      else KMessageBox::sorry(0,i18n("Your secret key could not be exported\nCheck the key..."));
    }

}


void listKeys::slotexport()
{
  /////////////////////  export key
  if (keysList2->currentItem()==NULL) return;
  char gpgcmd[1024] = "\0",line[130]="";
  exportresult="";

  QString key=keysList2->currentItem()->text(0);

  QString sname=key.section('@',0,0);
  sname=sname.section('.',0,0);
  sname.append(".asc");
  sname.prepend(QString(QDir::homeDirPath()+"/"));


  popupName *dial=new popupName(this,i18n("Export public key to"),sname,true);
  /////////////   open export dialog (KgpgExport, see begining of this file)
  dial->exec();

  if (dial->result()==true)
    {
      ////////////////////////// export to file
      FILE *fp;
      QString expname;
      expname="";
      if (dial->getfmode()==true)  expname=dial->getfname();
      //    if (dial->getmailmode()==true) expname=sname;

      strcat(gpgcmd,"gpg --no-tty --export --armor ");
      strcat(gpgcmd,key);
      QFile fgpg(expname);
      if (expname!="")
        {
          strcat(gpgcmd,QString(" > "+expname));
          if (fgpg.exists()) fgpg.remove();
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
          ////////////////////////////////  copy key to editor
          KgpgApp *win=(KgpgApp *) parent();
          win->view->editor->setText(tst);
        }

      /////

      if (expname!="")
        {
          if (fgpg.exists())
            {
              QString mess=i18n("Your public key \"%1\" was successfully exported\n").arg(dial->getfname());
              KMessageBox::information(0,mess);
            }
          else KMessageBox::sorry(0,i18n("Your public key could not be exported\nCheck the key..."));
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
  if (keysList2->currentItem()==NULL) return;
  /////////////   open a key info dialog (KgpgKeyInfo, see begining of this file)
  QString key=keysList2->currentItem()->text(0);
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
  if (keysList2->currentItem()==NULL) return;
  bool islocal;
  QString key,key2;

  key=keysList2->currentItem()->text(0);
  if (key!="")
    {
      //////////////////  open a key selection dialog (KgpgSelKey, see begining of this file)
      KgpgSelKey *opts=new KgpgSelKey(this);

      opts->exec();
      if (opts->result()==true)
        {
          key2=opts->getkey();
          islocal=opts->getlocal();
        }
      else
        {
          delete opts;
          return;
        }
      delete opts;

    }

  //KMessageBox::sorry(0,QString(key2+"\n"+key));

  /////////////////   ugly console trick...
  KProcess *conprocess=new KProcess();
  *conprocess<< "konsole"<<"-e"<<"gpg";
  *conprocess<<"--no-secmem-warning"<<"-u"<<key2;
  if (islocal==false) *conprocess<<"--sign-key"<<key;
  else *conprocess<<"--lsign-key"<<key;
  QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(refreshkey()));
  conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::slotedit()
{
if (keysList2->currentItem()==NULL) return;


FILE *pass;
          int status;
          pid_t pid;
          QString tst;
QString key=keysList2->currentItem()->text(5);
          char line[130],gpgcmd[200]="";

          //////////   fork process
          pid = fork ();
          if (pid == 0)  //////////  child process =console
            {
	    strcat(gpgcmd,"konsole -e gpg --no-secmem-warning --edit-key ");
	    strcat(gpgcmd,key);
	    strcat(gpgcmd," help");
              pass=popen(gpgcmd,"r");
              while ( fgets( line, sizeof(line), pass))
                tst+=line;
              pclose(pass);
            }
          else if (waitpid (pid, &status, 0) != pid)  ////// parent process wait for end of child
            status = -1;

      refreshkey();
}

void listKeys::slotprocresult(KProcess *p)
{
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


          //// create temp file

          //Subkey-Type:%3\nSubkey-Length:%4\n
          if (ktype=="ElGamal") filecont="Key-Type: 20\n";
          else if (ktype=="RSA") filecont="Key-Type: 1\n";
          else
            {
              filecont=QString("Key-Type:DSA\n");
              filecont+="Subkey-Type: ELG-E\n";
              filecont+=QString("Subkey-Length:%1\n").arg(ksize);
            }
          filecont+=QString("Key-Length:%1\n").arg(ksize);
          filecont+=QString("Name-Real:%1\n").arg(kname);
          if (kcomment!="") filecont+=QString("Name-Comment:%1\n").arg(kcomment);
          filecont+=QString("Name-Email:%1\n").arg(kmail);
          if (kexp==0) filecont+=QString("Expire-Date:0\n");
          if (kexp==1) filecont+=QString("Expire-Date:%1\n").arg(knumb);
          if (kexp==2) filecont+=QString("Expire-Date:%1w\n").arg(knumb);
          if (kexp==3) filecont+=QString("Expire-Date:%1m\n").arg(knumb);
          if (kexp==4) filecont+=QString("Expire-Date:%1y\n").arg(knumb);
          filecont+="%commit\n";
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


              QFile f;

              //// put encrypted data in a tempo file

              f.setName("kgpg.tmp");
              if ( !f.open( IO_WriteOnly ) )
                {
                  KMessageBox::sorry(0,i18n("Error writing temp file"));
                  return;
                }
              QTextStream t( &f );
              t << filecont;
              f.close();

              FILE *pass;
              int ppass[2];

              pipe(ppass);
              pass = fdopen(ppass[1], "w");
              fwrite(password, sizeof(char), strlen(password), pass);
              fwrite("\n", sizeof(char), 1, pass);
              fclose(pass);

              KProcIO *proc=new KProcIO();
              QString res;
              res.setNum(ppass[0]);

              *proc<<"gpg"<<"--no-tty"<<"--no-secmem-warning"<<"--batch"<<"--passphrase-fd"<<res<<"--gen-key"<<"-a"<<"kgpg.tmp";

              /////////  when process ends, update dialog infos
              QObject::connect(proc, SIGNAL(processExited(KProcess *)),this, SLOT(genover(KProcess *)));
              proc->start(KProcess::NotifyOnExit);
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

void listKeys::genover(KProcess *p)
{

  FILE *fp;
  char shredcmd[200]="";
  strcat(shredcmd,"shred -zu kgpg.tmp");
  fp = popen(shredcmd, "r");
  pclose(fp);

  refreshkey();
  delete pop;
}


void listKeys::deleteseckey()
{
  //////////////////////// delete a key
  QString res=keysList2->currentItem()->text(0);

  int result=KMessageBox::warningContinueCancel(this,i18n("Delete SECRET KEY pair %1 ?\nDeleting this key pair means you will never be able to decrypt files encrypted with this key anymore!!!").arg(res),i18n("Warning"),i18n("Delete"));
  if (result==KMessageBox::Cancel) return;

  KProcess *conprocess=new KProcess();
  *conprocess<< "konsole"<<"-e"<<"gpg";
  *conprocess<<"--no-secmem-warning"<<"--delete-secret-key"<<keysList2->currentItem()->text(5);
  QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(deletekey()));
  conprocess->start(KProcess::NotifyOnExit,KProcess::AllOutput);
}

void listKeys::confirmdeletekey()
{
  QString res=keysList2->currentItem()->text(0);

  int result=KMessageBox::warningContinueCancel(this,i18n("Delete public key %1 ?").arg(res),i18n("Warning"),i18n("Delete"));
  if (result==KMessageBox::Cancel) return;
  else deletekey();

}

void listKeys::deletekey()
{
  //////////////////////// delete a key
  FILE *fp;
  char gpgcmd[1024] = "\0";
  QString tst;
  strcat(gpgcmd,"gpg --no-tty --no-secmem-warning --batch --yes --delete-key ");
  strcat(gpgcmd,keysList2->currentItem()->text(5));

  fp=popen(gpgcmd,"r");
  pclose(fp);
  refreshkey();
}

void listKeys::slotImportKey()
{
  /////////////      import a key
  KURL url=KFileDialog::getOpenURL(QString::null,i18n("*.asc|*.asc files"), this,i18n("Select key file to import"));

  if(!url.isEmpty())
    {
      message="";
      KProcIO *conprocess=new KProcIO();
      *conprocess<< "gpg";
      *conprocess<<"--no-tty"<<"--no-secmem-warning"<<"--allow-secret-key-import"<<"--import"<<url.path();
      QObject::connect(conprocess, SIGNAL(processExited(KProcess *)),this, SLOT(slotprocresult(KProcess *)));
      QObject::connect(conprocess, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
      conprocess->start(KProcess::NotifyOnExit,true);
    }
}



void listKeys::refreshkey()
{
  ////////   update display of keys in main management window
  FILE *fp;
  QString tst;
  char line[130];
  keynames=NULL;
  keysList2->clear();

  char gpgcmd2[1024] = "\0";
  FILE *fp2;
  QString block="idre";
  issec="";
  secretList="";
  strcat(gpgcmd2,"gpg --no-secmem-warning --no-tty --list-secret-keys ");
  //strcat(gpgcmd2,tst);
  fp2 = popen(gpgcmd2, "r");
  while ( fgets( line, sizeof(line), fp2))  issec+=line;
  pclose(fp2);

  fp = popen("gpg --no-secmem-warning --no-tty --with-colon --list-keys", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;

      if (tst.find("pub",0,FALSE)!=-1)
        {
          const QString size=tst.section(':',2,2);
          const QString creat=tst.section(':',5,5);
          const QString tid=tst.section(':',4,4);
          QString id=QString("0x"+tid.right(8));
          const QString trust=tst.section(':',1,1);
          QString val=tst.section(':',6,6);
          if (val=="") val="Unlimited";
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
              keynames+=QString(id+","+tst);
              tst=tst.section('<',1,1);
              tst=tst.section('>',0,0);
              QString isbold="";
              if (id==defKey) isbold="on";
              UpdateViewItem *item=new UpdateViewItem(keysList2,tst,tr,val,size,creat,id,isbold);
              if (issec.find(id.right(8),0,FALSE)!=-1)
                {
                  if ((id==defKey) && (block.find(tr)==-1))   item->setPixmap(0,dkeyPair);
                  else   item->setPixmap(0,keyPair);

                  secretList+=tst;
                }
              else
                if ((id==defKey) && (block.find(tr)==-1)) item->setPixmap(0,dkeySingle);
                else item->setPixmap(0,keySingle);
            }
        }
    }
  pclose(fp);
  keysList2->setSelected(keysList2->firstChild(),true);
}
