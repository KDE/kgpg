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

#include <qptrlist.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qiconset.h>
#include <qbuttongroup.h>

#include "popuppublic.h"
#include "kgpgview.h"
#include "kgpg.h"

/////////////////   klistviewitem special

class UpdateViewItem2 : public KListViewItem
{
public:
    UpdateViewItem2(QListView *parent, QString tst);
    virtual void paintCell(QPainter *p, const QColorGroup &cg,int col, int width, int align);
};

UpdateViewItem2::UpdateViewItem2(QListView *parent, QString tst)
    : KListViewItem(parent)
{
setText(0,tst);
}


void UpdateViewItem2::paintCell(QPainter *p, const QColorGroup &cg,int column, int width, int alignment)
{
 if (column==0)
 {
        QFont font(p->font());
        font.setBold(true);
        p->setFont(font);
 }
    KListViewItem::paintCell(p, cg, column, width, alignment);
}

///////////////  main view

popupPublic::popupPublic(QWidget *parent, const char *name,QString sfile,bool filemode):QDialog(parent,name,TRUE)
{
  QString caption(i18n("Encryption"));
  config=kapp->config();
  config->setGroup("General Options");
  bool isascii=config->readBoolEntry("Ascii armor",true);
  bool istrust=config->readBoolEntry("Allow untrusted keys",false);
  bool hideid=config->readBoolEntry("Hide user id",false);
  displayMailFirst=config->readBoolEntry("display mail first",true);
  //pgpcomp=config->readBoolEntry("PGP compatibility",false);
  encryptToDefault=config->readBoolEntry("encrypt to default key",false);
  defaultKey=config->readEntry("default key");
  allowcustom=config->readBoolEntry("allow custom option",false);
  if (allowcustom) customOptions=config->readEntry("custom option");
  
//  encryptfileto=config->readBoolEntry("encrypt files to",false);
//  filekey=config->readEntry("file key");


defaultName="";


  KIconLoader *loader = KGlobal::iconLoader();

  keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
  keySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);

  //setMinimumSize(300,120);
  setCaption(caption);

  fmode=filemode;

  keysList = new KListView( this );
  keysList->setRootIsDecorated(true);
  
  keysList->setShowSortIndicator(true);
  keysList->setFullWidth(true);
  keysList->setSelectionModeExt(KListView::Extended);

  QVBoxLayout *vbox=new QVBoxLayout(this,3);

  if (sfile.isEmpty())
  keysList->addColumn(i18n("Encryption key(s):"));
  else
      keysList->addColumn(i18n("Encryption key(s) for %1:").arg(sfile));
  
  boutonboxoptions=new QButtonGroup(5,Qt::Vertical ,this,0);  

  CBarmor=new QCheckBox(i18n("ASCII armored encryption"),boutonboxoptions);
  CBuntrusted=new QCheckBox(i18n("Allow encryption with untrusted keys"),boutonboxoptions);
  CBhideid=new QCheckBox(i18n("Hide user id"),boutonboxoptions);

//boutonboxoptions->insert(CBarmor);
//boutonboxoptions->insert(CBuntrusted);

  QWhatsThis::add(keysList,i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
  QWhatsThis::add(CBarmor,i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));
  QWhatsThis::add(CBhideid,i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                                 "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because " 
								 "all available secret keys are tried."));
  QWhatsThis::add(CBuntrusted,i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                                 "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                                 "box enables you to use any key, even if it has not be signed."));

  if (filemode)
  {
  CBshred=new QCheckBox(i18n("Shred source file"),boutonboxoptions);
  QWhatsThis::add(CBshred,i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));

  CBsymmetric=new QCheckBox(i18n("Symmetrical encryption"),boutonboxoptions);
  QWhatsThis::add(CBsymmetric,i18n("<b>Symmetrical encryption</b>: encryption doesn't use keys. You just need to give a password "
                                 "to encrypt/decrypt the file"));
  QObject::connect(CBsymmetric,SIGNAL(toggled(bool)),this,SLOT(isSymetric(bool)));
  }

KButtonBox *boutonbox=new KButtonBox(this,KButtonBox::Horizontal,15,12);
  bouton0=boutonbox->addButton(i18n("&Options"),this,SLOT(toggleOptions()),TRUE);
  bouton0->setIconSet(QIconSet(KGlobal::iconLoader()->loadIcon("up",KIcon::Small)));
  boutonbox->addStretch(1);
  bouton1=boutonbox->addButton(i18n("&Encrypt"),this,SLOT(crypte()),TRUE);
  bouton2=boutonbox->addButton(i18n("&Cancel"),this,SLOT(annule()),TRUE);
  boutonbox->layout();
  bouton1->setDefault(true);
  if (isascii) CBarmor->setChecked(true);
  if (istrust) CBuntrusted->setChecked(true);
  if (hideid) CBhideid->setChecked(true);

  vbox->addWidget(keysList);
  if (allowcustom)
  {
  QHButtonGroup *bGroup = new QHButtonGroup(this);
  bGroup->setLineWidth(0);
  bGroup->setInsideMargin(2);
  (void) new QLabel(i18n("Custom option "),bGroup);
  KLineEdit *optiontxt=new KLineEdit(bGroup);
  optiontxt->setText(customOptions);
  QWhatsThis::add(optiontxt,i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));
  vbox->addWidget(bGroup);
  QObject::connect(optiontxt,SIGNAL(textChanged ( const QString & )),this,SLOT(customOpts(const QString & )));
  }
  vbox->addWidget(boutonboxoptions);
  boutonboxoptions->hide();
  vbox->addWidget(boutonbox);

  QObject::connect(keysList,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(precrypte()));
  QObject::connect(CBuntrusted,SIGNAL(toggled(bool)),this,SLOT(refresh(bool)));


 char line[200]="\0";
 FILE *fp2;
 seclist="";

              fp2 = popen("gpg --no-secmem-warning --no-tty --list-secret-keys ", "r");
              while ( fgets( line, sizeof(line), fp2))  seclist+=line;
              pclose(fp2);

trusted=istrust;
refreshkeys();
}


void popupPublic::toggleOptions()
{
if (boutonboxoptions->isVisible())
{
boutonboxoptions->hide();
bouton0->setIconSet(QIconSet(KGlobal::iconLoader()->loadIcon("up",KIcon::Small)));
}
else
{
boutonboxoptions->show();
bouton0->setIconSet(QIconSet(KGlobal::iconLoader()->loadIcon("down",KIcon::Small)));
}
}


void popupPublic::enable()
{
QListViewItem *current = keysList->firstChild();
if (current==NULL) return;

	current->setVisible(true);
        while ( current->nextSibling() )
	{
current = current->nextSibling();
current->setVisible(true);
	}
}

void popupPublic::sort()
{
bool reselect=false;
QString block=i18n("Undefined")+" , "+i18n("?")+" , "+i18n("Unknown")+" , "+i18n("None");
QListViewItem *current = keysList->firstChild();
if (current==NULL) return;

	QString trust=current->firstChild()->text(0);
	trust=trust.section(',',1,1);
	trust=trust.section(':',1,1);
	trust=trust.stripWhiteSpace();
	if (block.find(trust,0,false)!=-1)
	{
	if (current->isSelected()) {current->setSelected(false);reselect=true;}
	current->setVisible(false);
	}
            while ( current->nextSibling() )
                {
		current = current->nextSibling();
		QString trust=current->firstChild()->text(0);
	trust=trust.section(',',1,1);
	trust=trust.section(':',1,1);
	trust=trust.stripWhiteSpace();
		if (block.find(trust,0,false)!=-1)
		{
		if (current->isSelected()) {current->setSelected(false);reselect=true;}
		current->setVisible(false);
		}
		}

if (reselect)
{
QListViewItem *firstvisible;
firstvisible=keysList->firstChild();
while (firstvisible->isVisible()!=true)
{
firstvisible=firstvisible->nextSibling();
if (firstvisible==NULL) return;
}
keysList->setSelected(firstvisible,true);
keysList->setCurrentItem(firstvisible);
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
if (state==true) enable();
else sort();
}

void popupPublic::refreshkeys()
{
 KProcIO *encid=new KProcIO();
  *encid << "gpg"<<"--no-secmem-warning"<<"--no-tty"<<"--with-colon"<<"--list-keys";
  /////////  when process ends, update dialog infos
    QObject::connect(encid, SIGNAL(processExited(KProcess *)),this, SLOT(slotpreselect()));
    QObject::connect(encid, SIGNAL(readReady(KProcIO *)),this, SLOT(slotprocread(KProcIO *)));
    encid->start(KProcess::NotifyOnExit,true);
}

void popupPublic::slotpreselect()
{
if (trusted==false) sort();
if (encryptToDefault==true)
{
keysList->setSelected(keysList->findItem(defaultName,0),true);
keysList->setCurrentItem(keysList->findItem(defaultName,0));
}
else
{
QListViewItem *firstvisible;
firstvisible=keysList->firstChild();
if (firstvisible==NULL) return;
while (firstvisible->isVisible()!=true)
{
firstvisible=firstvisible->nextSibling();
if (firstvisible==NULL) return;
}
keysList->setSelected(firstvisible,true);
keysList->setCurrentItem(firstvisible);
}
}


void popupPublic::slotprocread(KProcIO *p)
{
///////////////////////////////////////////////////////////////// extract  encryption keys
bool dead;
QString tst;

  while (p->readln(tst)!=-1)
  {
   
  //tst=QString::fromUtf8(tst);
       if (tst.startsWith("pub"))
        {
	dead=false;
          const QString trust=tst.section(':',1,1);
          QString val=tst.section(':',6,6);
	  QString id=QString("0x"+tst.section(':',4,4).right(8));
          if (val=="") val=i18n("Unlimited");
          QString tr;
          switch( trust[0] )
            {
            case 'o':
              tr=i18n("Unknown");
              break;
            case 'i':
              tr=i18n("Invalid");
	      dead=true;
              break;
            case 'd':
              tr=i18n("Disabled");
	      dead=true;
              break;
            case 'r':
              tr=i18n("Revoked");
	      dead=true;
              break;
            case 'e':
              tr=i18n("Expired");
	      dead=true;
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
tst=tst.section(':',9,9);

if ((!dead) && (!tst.isEmpty()))
	{
	if ((id==defaultKey) && (encryptToDefault))
	      {
	      defaultName=extractKeyName(tst);
	      UpdateViewItem2 *item=new UpdateViewItem2(keysList,defaultName);
	      KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, validity: %3").arg(id).arg(tr).arg(val));
	      sub->setSelectable(false);
	      if (seclist.find(tst,0,FALSE)!=-1) item->setPixmap(0,keyPair);
              else item->setPixmap(0,keySingle);
	      }
	      else
	      {
	      KListViewItem *item=new KListViewItem(keysList,extractKeyName(tst));
	      KListViewItem *sub= new KListViewItem(item,i18n("ID: %1, trust: %2, validity: %3").arg(id).arg(tr).arg(val));
	      sub->setSelectable(false);
	      if (seclist.find(tst,0,FALSE)!=-1) item->setPixmap(0,keyPair);
              else item->setPixmap(0,keySingle);
	      }
	  }
}
}
}

QString popupPublic::extractKeyName(QString fullName)
{
QString kMail;
if (fullName.find("<")!=-1)
{
kMail=fullName.section('<',-1,-1);
kMail.truncate(kMail.length()-1);
}
QString kName=fullName.section('<',0,0);
if (kName.find("(")!=-1) kName=kName.section('(',0,0);
if (displayMailFirst) return QString(kMail+" ("+kName+")").stripWhiteSpace();
return QString(kName+" ("+kMail+")").stripWhiteSpace();
}

void popupPublic::annule()
{
////////  cancel & close dialog
  reject();
}


void popupPublic::precrypte()
{
//////   emit selected data
if (keysList->currentItem()->depth()==0) crypte();
}

void popupPublic::crypte()
{
//////   emit selected data

QString res,userid;
QPtrList<QListViewItem> list=keysList->selectedItems();

for ( uint i = 0; i < list.count(); ++i )
if ( list.at(i) )
{
userid=list.at(i)->firstChild()->text(0);
	userid=userid.section(',',0,0);
	userid=userid.section(':',1,1);
	userid=userid.stripWhiteSpace();
res+=" "+userid;
}
if (res.isEmpty()) return;
if ((encryptToDefault) && (res.find(defaultKey)==-1)) res+=" "+defaultKey;
QString returnOptions;
if (CBuntrusted->isChecked()) returnOptions=" --always-trust ";
if (CBarmor->isChecked()) returnOptions+=" --armor ";
if (CBhideid->isChecked()) returnOptions+=" --throw-keyid ";
if ((allowcustom) && (!customOptions.stripWhiteSpace().isEmpty())) returnOptions+=customOptions;

if (fmode)
	emit selectedKey(res,returnOptions,CBshred->isChecked(),CBsymmetric->isChecked());
else emit selectedKey(res,returnOptions,false,false);
  accept();
}

#include "popuppublic.moc"
