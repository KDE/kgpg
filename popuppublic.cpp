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
  QLabel *labeltxt;
  QString caption(i18n("Encryption"));
  config=kapp->config();
  config->setGroup("General Options");
  bool isascii=config->readBoolEntry("Ascii armor",true);
  bool istrust=config->readBoolEntry("Allow untrusted keys",false);
  displayMailFirst=config->readBoolEntry("display mail first",true);
  //pgpcomp=config->readBoolEntry("PGP compatibility",false);
  encryptToDefault=config->readBoolEntry("encrypt to default key",false);
  defaultKey=config->readEntry("default key");
//  encryptfileto=config->readBoolEntry("encrypt files to",false);
//  filekey=config->readEntry("file key");


defaultName="";


  KIconLoader *loader = KGlobal::iconLoader();

  keyPair=loader->loadIcon("kgpg_key2",KIcon::Small,20);
  keySingle=loader->loadIcon("kgpg_key1",KIcon::Small,20);
  dkeyPair=loader->loadIcon("kgpg_dkey2",KIcon::Small,20);
  dkeySingle=loader->loadIcon("kgpg_dkey1",KIcon::Small,20);

  //setMinimumSize(300,120);
  setCaption(caption);

  fmode=filemode;

  keysList = new KListView( this );
  keysList->setRootIsDecorated(true);
  keysList->addColumn( i18n( "Keys" ) );
  //keysList->addColumn( i18n( "Trust" ) );
  //keysList->addColumn( i18n( "Validity" ) );
  keysList->setShowSortIndicator(true);
  keysList->setFullWidth(true);
  keysList->setSelectionModeExt(KListView::Extended);

  QVBoxLayout *vbox=new QVBoxLayout(this,3);

  if (sfile=="")
    labeltxt=new QLabel(i18n("Choose encryption key(s):"),this);
  else
    {
      caption=i18n("Choose encryption key(s) for %1:").arg(sfile);
      labeltxt=new QLabel(caption,this);
    }

  KButtonBox *boutonbox=new KButtonBox(this,KButtonBox::Horizontal,15,10);

  checkbox1=new QCheckBox(i18n("ASCII Armored encryption"),this);
  checkbox2=new QCheckBox(i18n("Allow encryption with untrusted keys"),this);

   QWhatsThis::add(keysList,i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
  QWhatsThis::add(checkbox1,i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));
  QWhatsThis::add(checkbox2,i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
"marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
"box enables you to use any key, even if it has not be signed."));

  if (filemode==true)
  {
  checkbox3=new QCheckBox(i18n("Shred source file"),this);
  QWhatsThis::add(checkbox3,i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));

  checkbox4=new QCheckBox(i18n("Symmetrical encryption"),this);
  QWhatsThis::add(checkbox3,i18n("<b>Symmetrical encryption</b>: encryption doesn't use keys. You just need to give a password "
  "to encrypt/decrypt the file"));
  }

  boutonbox->addStretch(1);
  bouton1=boutonbox->addButton(i18n("&Encrypt"),TRUE);
  bouton2=boutonbox->addButton(i18n("&Cancel"),TRUE);

  if (isascii) checkbox1->setChecked(true);
  if (istrust) checkbox2->setChecked(true);

  vbox->addWidget(labeltxt);
  vbox->addWidget(keysList);
  vbox->addWidget(checkbox1);
  vbox->addWidget(checkbox2);
  if (filemode==true)
  {
  vbox->addWidget(checkbox3);
  vbox->addWidget(checkbox4);
  QObject::connect(checkbox4,SIGNAL(toggled(bool)),this,SLOT(isSymetric(bool)));
  }
  vbox->addWidget(boutonbox);

  QObject::connect(keysList,SIGNAL(doubleClicked(QListViewItem *,const QPoint &,int)),this,SLOT(precrypte()));
  QObject::connect(bouton1,SIGNAL(clicked()),this,SLOT(crypte()));
  QObject::connect(bouton2,SIGNAL(clicked()),this,SLOT(annule()));
  QObject::connect(checkbox2,SIGNAL(toggled(bool)),this,SLOT(refresh(bool)));


 char gpgcmd2[1024] = "\0",line[200]="\0";
 FILE *fp2;
 seclist="";

              strcat(gpgcmd2,"gpg --no-secmem-warning --no-tty --list-secret-keys ");
              fp2 = popen(gpgcmd2, "r");
              while ( fgets( line, sizeof(line), fp2))  seclist+=line;
              pclose(fp2);

trusted=istrust;
refreshkeys();
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
checkbox2->setEnabled(!state);
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

if ((dead==false) && (tst!=""))
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

QString res="",userid;
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
if (res=="") {reject();return;}
if ((encryptToDefault==true) && (res.find(defaultKey)==-1)) res+=" "+defaultKey;
if (fmode==true)
    emit selectedKey(res,checkbox2->isChecked(),checkbox1->isChecked(),checkbox3->isChecked(),checkbox4->isChecked());
  else emit selectedKey(res,checkbox2->isChecked(),checkbox1->isChecked(),false,false);
  accept();
}

#include "popuppublic.moc"
