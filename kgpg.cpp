/***************************************************************************
                          kgpg.cpp  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#include <qlabel.h>
#include <qpixmap.h>
#include <qclipboard.h>
#include <qfile.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kcombobox.h>
#include <kwin.h>
#include <kprocess.h>
#include <kprocio.h>
#include <qcursor.h>
//#include <qframe.h>
#include <qwidget.h>
#include <qtooltip.h>
#include <kaboutapplication.h>


#include "kgpg.h"


#define ABOUT_ITEM    30
#define HELP_ITEM    40
#define QUIT_ITEM    50
#define CONFIG_ITEM  60
#define CLIPDECRYPT_ITEM   70
#define CLIPENCRYPT_ITEM   80
#define KEYMANAGER_ITEM   90
#define EDITOR_ITEM   100
#define KEYSERVER_ITEM   110

MyView::MyView( QWidget *parent, const char *name )
    : QLabel( parent, name )
{

KAction *saveDecrypt = new KAction(i18n("&Decrypt && Save File"),"decrypted",0,this, SLOT(decryptDroppedFile()),this,"decrypt_file");
KAction *showDecrypt = new KAction(i18n("&Show Decrypted File"),"edit",0,this, SLOT(showDroppedFile()),this,"show_file");
KAction *encrypt = new KAction(i18n("&Encrypt File"),"encrypted",0,this, SLOT(encryptDroppedFile()),this,"encrypt_file");
KAction *sign = new KAction(i18n("&Sign File"), "signature",0,this, SLOT(signDroppedFile()),this,"sign_file");
//QToolTip::add(this,i18n("KGpg drag & drop encryption applet"));

m_popup = new KPopupMenu(0L, "main_menu");
conf_popup = new KPopupMenu(0L, "config_menu");
ksConfig=kapp->config();
readOptions();
init();
if (tipofday) KTipDialog::showTip(this, "kgpg/tips", true);

setPixmap(KGlobal::iconLoader()->loadIcon("kgpg",KIcon::User,22));
resize(24,24);
setAcceptDrops(true);

  droppopup=new QPopupMenu();
  showDecrypt->plug(droppopup);
  saveDecrypt->plug(droppopup);
  
  udroppopup=new QPopupMenu();
  encrypt->plug(udroppopup);
  sign->plug(udroppopup);
  
    connect(m_popup, SIGNAL(activated(int)), SLOT(clickedMenu(int)));
	connect(conf_popup, SIGNAL(activated(int)), SLOT(clickedConfMenu(int)));

	m_keyManager=0L;
 
}

MyView::~MyView()
{
if (m_popup)
{
delete m_popup;
m_popup = 0;
}
if (conf_popup)
{
delete conf_popup;
conf_popup = 0;
}
if (droppopup)
{
delete droppopup;
droppopup = 0;
}
if (udroppopup)
{
delete udroppopup;
udroppopup = 0;
}
}

void MyView::init()
{
  m_popup->clear();
  m_popup->insertTitle( SmallIcon( "kgpg" ),
                        i18n("KGpg Applet"));

if (showeclip)
m_popup->insertItem(i18n("&Encrypt Clipboard"), CLIPENCRYPT_ITEM );
if (showdclip)
m_popup->insertItem(i18n("&Decrypt Clipboard"), CLIPDECRYPT_ITEM );
m_popup->insertSeparator();
if (showomanager)
m_popup->insertItem( SmallIcon("kgpg_manage"),
			i18n("Open &Key Manager"), KEYMANAGER_ITEM );
if (showoeditor)
m_popup->insertItem( SmallIcon("edit"),
			i18n("&Open Editor"), EDITOR_ITEM );

if (showserver)
{
m_popup->insertSeparator();
m_popup->insertItem( SmallIcon("network"),
			i18n("Key&server Dialog"), KEYSERVER_ITEM );
}

///////////////////////////////////////////////////////////////////

  conf_popup->clear();
  conf_popup->insertTitle( SmallIcon( "kgpg" ),
                        i18n("KGpg Applet"));

 conf_popup->insertItem(SmallIcon("configure"), i18n("&Configure KGpg..."), CONFIG_ITEM );
 conf_popup->insertItem(SmallIcon("kgpg"), i18n("&About KGpg"), ABOUT_ITEM );
 conf_popup->insertItem(SmallIcon("help"), i18n("&Help"), HELP_ITEM );
conf_popup->insertSeparator(); 
  conf_popup->insertItem(SmallIcon("exit"), i18n("&Quit"), QUIT_ITEM );
}

void MyView::showPopupMenu( QPopupMenu *menu )
{
	Q_ASSERT( menu != 0L );

    menu->move(-1000,-1000);
    menu->show();
    menu->hide();

        QPoint g = QCursor::pos();
        if ( menu->height() < g.y() )
            menu->popup(QPoint( g.x(), g.y() - menu->height()));
        else
            menu->popup(QPoint(g.x(), g.y()));
}


void MyView::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() == RightButton)
        showPopupMenu( conf_popup );
	else  showPopupMenu( m_popup );
}

void MyView::clickedMenu(int id)
{
    switch ( id ) {
    case -1:
        break;
	case KEYSERVER_ITEM:
        openKeyServer();
        break;
    case KEYMANAGER_ITEM:
        openKeyManager();
        break;
    case EDITOR_ITEM:
        openEditor();
        break;
	case CLIPENCRYPT_ITEM:
        clipEncrypt();
        break;
	case CLIPDECRYPT_ITEM:
        clipDecrypt();
        break;
	}
}


void MyView::clickedConfMenu(int id)
{
    switch ( id ) {
    case -1:
        break;
	case HELP_ITEM: 
        help();
        break;
	case ABOUT_ITEM: 
        about();
        break;
	case CONFIG_ITEM: 
        preferences();
        break;
	case QUIT_ITEM: 
	{
	 int autoStart = KMessageBox::questionYesNoCancel( 0, i18n("Should KGpg start automatically\nwhen you login?"), i18n("Automatically Start KGpg?") );
        ksConfig->setGroup("Applet");
        if ( autoStart == KMessageBox::Yes )
            ksConfig->writeEntry("AutoStart", true);
        else if ( autoStart == KMessageBox::No) {
            ksConfig->writeEntry("AutoStart", false);
        }else  // cancel chosen don't quit
	    break;
		ksConfig->sync();
        kapp->exit();
        break;
	
		}
    }
}


void  MyView::openKeyServer() 
{
keyServer *ks=new keyServer(0);
ks->exec();
delete ks;
}

void  MyView::openKeyManager() 
{
	if(!m_keyManager)
	{
		m_keyManager = new listKeys(0, "key_manager",WDestructiveClose);
		connect( m_keyManager , SIGNAL( destroyed() ) , this, SLOT( slotKeyManagerClosed()));
	}
	m_keyManager->show();
	KWin::setOnDesktop( m_keyManager->winId() , KWin::currentDesktop() );  //set on the current desktop
	m_keyManager->raise();  // set on top
}

void  MyView::clipEncrypt() 
{
popupPublic *dialoguec=new popupPublic(this, "public_keys", 0,false);
connect(dialoguec,SIGNAL(selectedKey(QString &,QString,bool,bool)),this,SLOT(encryptClipboard(QString &,QString)));
dialoguec->show();
}

void  MyView::clipDecrypt() 
{
QString clippie=kapp->clipboard()->text().stripWhiteSpace();
if (clippie.startsWith("-----BEGIN PGP MESSAGE"))
{
   KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
   kgpgtxtedit->view->editor->setText(clippie);
   kgpgtxtedit->view->slotdecode();
   kgpgtxtedit->show();
}
else KMessageBox::sorry(this,i18n("No encrypted text found."));
}


void  MyView::openEditor()
{
 KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WType_Dialog);
 kgpgtxtedit->show();
}

void  MyView::encryptDroppedFile()
{
QString opts="";
KgpgLibrary *lib=new KgpgLibrary();
if (encryptfileto)
{
		  	if (untrusted) opts=" --always-trust ";
			if (ascii) opts+=" --armor ";
			if (hideid) opts+=" --throw-keyid ";
			if (pgpcomp) opts+=" --pgp6 ";
lib->slotFileEnc(droppedUrl,opts,filekey);
}
else lib->slotFileEnc(droppedUrl);
}


void  MyView::slotVerifyFile()
{
  ///////////////////////////////////   check file signature
 if (droppedUrl.isEmpty()) return;

QString sigfile="";
      //////////////////////////////////////       try to find detached signature.
if (!droppedUrl.filename().endsWith(".sig"))
{
      sigfile=droppedUrl.path()+".sig";
      QFile fsig(sigfile);
      if (!fsig.exists())
        {
          sigfile=droppedUrl.path()+".asc";
          QFile fsig(sigfile);
          //////////////   if no .asc or .sig signature file included, assume the file is internally signed
          if (!fsig.exists())
            sigfile="";
        }
}
else
{
sigfile=droppedUrl.path();
droppedUrl=KURL(sigfile.left(sigfile.length()-4));
}

      ///////////////////////// pipe gpg command
KgpgInterface *verifyFileProcess=new KgpgInterface();
verifyFileProcess->KgpgVerifyFile(droppedUrl,KURL(sigfile));
}


void  MyView::signDroppedFile() 
{

  //////////////////////////////////////   create a detached signature for a chosen file
 if (droppedUrl.isEmpty()) return;
 
 QString signKeyID;
      //////////////////   select a private key to sign file --> listkeys.cpp
	  KgpgSelKey *opts=new KgpgSelKey(0,"select_secret",false);
      if (opts->exec()==QDialog::Accepted) signKeyID=opts->getkeyID();
      else
        {
          delete opts;
          return;
        }
      delete opts;
  QString Options;
  if (ascii) Options=" --armor ";
  if (pgpcomp) Options+=" --pgp6 ";
 KgpgInterface *signFileProcess=new KgpgInterface();
 signFileProcess->KgpgSignFile(signKeyID,droppedUrl,Options);
}

void  MyView::decryptDroppedFile()
{
if (!droppedUrl.isLocalFile())
{
if (KMessageBox::warningContinueCancel(0,i18n("<qt><b>Remote file decryption</b>.<br>The remote file will now be copied to a temporary file to process decryption. This temporary file will be deleted after operation.</qt>"),0,KStdGuiItem::cont(),"RemoteFileWarning")!=KMessageBox::Continue) return;
showDroppedFile();
return;
}
QString oldname=droppedUrl.filename();
  if (oldname.endsWith(".gpg") || oldname.endsWith(".asc") || oldname.endsWith(".pgp"))
    oldname.truncate(oldname.length()-4);
  else oldname.append(".clear");
  KURL swapname(droppedUrl.directory(0,0)+oldname);
      QFile fgpg(swapname.path());
      if (fgpg.exists())
        {
          KgpgOverwrite *over=new KgpgOverwrite(0,"overwrite",swapname);
          if (over->exec()==QDialog::Accepted)
            swapname=KURL(swapname.directory(0,0)+over->getfname());
          else return;
        }

KgpgLibrary *lib=new KgpgLibrary();
lib->slotFileDec(droppedUrl,swapname,customDecrypt) ;
}

void  MyView::showDroppedFile()
{

KgpgApp *kgpgtxtedit = new KgpgApp(0, "editor",WDestructiveClose);
   kgpgtxtedit->view->editor->droppedfile(droppedUrl);
   kgpgtxtedit->show();
}


void  MyView::droppedfile (KURL url)
{
droppedUrl=url;
if (!url.isLocalFile())
{
if (KMessageBox::warningContinueCancel(0,i18n("<qt><b>Remote file dropped</b>.<br>The remote file will now be copied to a temporary file to process encryption/decryption. This temporary file will be deleted after operation.</qt>"),0,KStdGuiItem::cont(),"RemoteFileWarning")!=KMessageBox::Continue) return;
showDroppedFile();
return;
}
if ((url.path().endsWith(".asc")) || (url.path().endsWith(".pgp")) || (url.path().endsWith(".gpg")))
{
switch (efileDropEvent)
{
case 0:
	decryptDroppedFile();
	break;
case 1:
	showDroppedFile();
	break;
case 2:
	droppopup->exec(QCursor::pos ());
	break;
}
}
else if (url.path().endsWith(".sig"))
{
KgpgInterface *verifyFileProcess=new KgpgInterface();
verifyFileProcess->KgpgVerifyFile(url,"");
}
else switch (ufileDropEvent)
{
case 0:
	encryptDroppedFile();
	break;
case 1:
	signDroppedFile();
	break;
case 2:
	udroppopup->exec(QCursor::pos ());
	break;
}
}


void  MyView::droppedtext (QString inputText)
{

QClipboard *cb = QApplication::clipboard();
cb->setText(inputText);
if (inputText.startsWith("-----BEGIN PGP MESSAGE"))
{
clipDecrypt();
return;
}
if (inputText.startsWith("-----BEGIN PGP PUBLIC KEY")) 
{
int result=KMessageBox::warningContinueCancel(0,i18n("<p>The dropped text is a public key.<br>Do you want to import it ?</p>"),i18n("Warning"));
        if (result==KMessageBox::Cancel) return;
        else
        {
		KgpgInterface *importKeyProcess=new KgpgInterface();
  		importKeyProcess->importKey(inputText,false);
        return;
}
}
clipEncrypt();
}


void  MyView::dragEnterEvent(QDragEnterEvent *e)
{
e->accept (QUrlDrag::canDecode(e) || QTextDrag::canDecode (e));
}


void  MyView::dropEvent (QDropEvent *o) 
{
  QStringList list;
  QString text;
  if ( QUrlDrag::decodeToUnicodeUris( o, list ) ) droppedfile(KURL(list.first()));
  else if ( QTextDrag::decode(o, text) ) droppedtext(text);
}


void  MyView::readOptions()
{
ksConfig->setGroup("Applet");
ufileDropEvent=ksConfig->readNumEntry("unencrypted drop event",0);
efileDropEvent=ksConfig->readNumEntry("encrypted drop event",2);
showeclip=ksConfig->readBoolEntry("show encrypt clip",true);
showdclip=ksConfig->readBoolEntry("show decrypt clip",true);
showoeditor=ksConfig->readBoolEntry("show open editor",true);
showomanager=ksConfig->readBoolEntry("show open manager",true);
showserver=ksConfig->readBoolEntry("show server",true);

ksConfig->setGroup("General Options");
encryptfileto=ksConfig->readBoolEntry("encrypt files to",false);
filekey=ksConfig->readEntry("file key");
ascii=ksConfig->readBoolEntry("Ascii armor",true);
untrusted=ksConfig->readBoolEntry("Allow untrusted keys",false);
hideid=ksConfig->readBoolEntry("Hide user ID",false);
pgpcomp=ksConfig->readBoolEntry("PGP compatibility",false);
customDecrypt=ksConfig->readEntry("custom decrypt");
if (ksConfig->readBoolEntry("selection clip",false))
{
if (kapp->clipboard()->supportsSelection())
kapp->clipboard()->setSelectionMode(true);
}
else kapp->clipboard()->setSelectionMode(false);

if (ksConfig->readBoolEntry("First run",true)) firstRun();
ksConfig->setGroup("TipOfDay");
tipofday=ksConfig->readBoolEntry("RunOnStart",true);

}

void  MyView::firstRun()
{
  FILE *fp;
  QString tst;
  char line[200];
  bool found=false;

kgpgOptions *opts=new kgpgOptions(this,0);
opts->checkMimes();
delete opts;

  fp = popen("gpg --no-tty --with-colon --list-secret-keys", "r");
  while ( fgets( line, sizeof(line), fp))
    {
      tst=line;
      if (tst.startsWith("sec"))
      found=true;
    }
  pclose(fp);
  if (!found)
    {
      int result=KMessageBox::questionYesNo(0,i18n("Welcome to KGPG.\nNo secret key was found on your computer.\nWould you like to create one now?"));
      if (result==3)
        {
          listKeys *creat=new listKeys(0);
          creat->slotgenkey();
          delete creat;
        }
    }
}

void  MyView::about()
{
    KAboutApplication dialog(kapp->aboutData());//_aboutData);
    dialog.exec();
}

void  MyView::help()
{
kapp->invokeHelp(0,"kgpg");
}


void  MyView::preferences()
{
kgpgOptions *opts=new kgpgOptions();
opts->exec();
delete opts;
readOptions();
init();
}

void MyView::slotKeyManagerClosed()
{
	m_keyManager=0L;
}



kgpgapplet::kgpgapplet()
    : KSystemTray()
{	
  w=new MyView(this);
   w->init();
   w->show();
}

kgpgapplet::~kgpgapplet()
{
if (w)
{
delete w;
w = 0;
}
}


KgpgAppletApp::KgpgAppletApp()
    : KUniqueApplication(), kgpg_applet( 0 )
{
}


KgpgAppletApp::~KgpgAppletApp()
{
if (kgpg_applet)
{
delete kgpg_applet;
kgpg_applet = 0;	
}
}

int KgpgAppletApp::newInstance()
{
KURL FileToOpen;
args = KCmdLineArgs::parsedArgs();

	if ( kgpg_applet )
	{
		kgpg_applet->show();
	}
	else
	{
		kgpg_applet = new kgpgapplet;
		kgpg_applet->show();
		/*if ( isRestored() && KMainWindow::canBeRestored(0) )
		{
			kgpg_applet->restore(0, FALSE);
		}*/
	}
	
////////////////////////   parsing of command line args
if (args->count()>0)
{
FileToOpen=args->url(0);
if (FileToOpen.isEmpty()) return 0;
kgpg_applet->w->droppedUrl=FileToOpen;
if (args->isSet("e")!=0)	kgpg_applet->w->encryptDroppedFile();
else if (args->isSet("s")!=0) kgpg_applet->w->showDroppedFile();
else if (args->isSet("S")!=0) kgpg_applet->w->signDroppedFile();	
else if (args->isSet("V")!=0) kgpg_applet->w->slotVerifyFile();
else if (FileToOpen.filename().endsWith(".sig")) kgpg_applet->w->slotVerifyFile();
else kgpg_applet->w->decryptDroppedFile();
}

return 0;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void MyView::encryptClipboard(QString &selec,QString encryptOptions)
{
QString clipContent=kapp->clipboard()->text();//=cb->text(QClipboard::Clipboard);   ///   QT 3.1 only

  if (clipContent)
  {
  if (pgpcomp) encryptOptions+=" --pgp6 ";
  encryptOptions+=" --armor ";
    
 if (selec==NULL) {KMessageBox::sorry(0,i18n("You have not chosen an encryption key."));}//exit(0);}
 
 KgpgInterface *txtEncrypt=new KgpgInterface();
connect (txtEncrypt,SIGNAL(txtencryptionfinished(QString)),this,SLOT(slotSetClip(QString)));
 txtEncrypt->KgpgEncryptText(clipContent,selec,encryptOptions);
QString cryptedClipboard;
if (clipContent.length()>300) cryptedClipboard=QString(clipContent.left(250).stripWhiteSpace())+"...\n"+QString(clipContent.right(40).stripWhiteSpace());
else cryptedClipboard=clipContent;


cryptedClipboard.replace(QRegExp("<"),"&lt;");   /////   disable html tags
cryptedClipboard.replace(QRegExp("\n"),"<br>");

#if (KDE_VERSION >= 310)
pop = new KPassivePopup( this);
pop->setView(i18n("Encrypted following text:"),cryptedClipboard,KGlobal::iconLoader()->loadIcon("kgpg",KIcon::Desktop));
		pop->setTimeout(3200);
	  	pop->show();	  
	  	QRect qRect(QApplication::desktop()->screenGeometry());
		int iXpos=qRect.width()/2-pop->width()/2;
		int iYpos=qRect.height()/2-pop->height()/2;
      	pop->move(iXpos,iYpos);
#else
	clippop = new QDialog( this,0,false,WStyle_Customize | WStyle_NormalBorder);
              QVBoxLayout *vbox=new QVBoxLayout(clippop,3);
              QLabel *tex=new QLabel(clippop);
              tex->setText(i18n("<b>Encrypted following text:</b>"));
			  QLabel *tex2=new QLabel(clippop);
			  //tex2->setTextFormat(Qt::PlainText);
			  tex2->setText(cryptedClipboard);
              vbox->addWidget(tex);
			  vbox->addWidget(tex2);
              clippop->setMinimumWidth(250);
              clippop->adjustSize();
			  clippop->show();
 QTimer::singleShot( 3200, this, SLOT(killDisplayClip()));
//KMessageBox::information(this,i18n("<b>Encrypted following text</b>:<br>")+cryptedClipboard);
#endif
 }
 else
{
KMessageBox::sorry(0,i18n("Clipboard is empty."));
//exit(0);
}
}

void MyView::slotSetClip(QString newtxt)
{
 if (!newtxt.isEmpty())
 {
 QClipboard *clip=QApplication::clipboard();
 clip->setText(newtxt);//,QClipboard::Clipboard);    QT 3.1 only
 //connect(kapp->clipboard(),SIGNAL(dataChanged ()),this,SLOT(expressQuit()));
 }
 //else expressQuit();
}

void MyView::killDisplayClip()
{
#if (KDE_VERSION < 310)
delete clippop;
#endif
}



/////////////////////////////////////////////////////////////////////////////

#include "kgpg.moc"


