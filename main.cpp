/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Tue Jul  2 12:31:38 GMT 2002
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

#include <stdlib.h>

#include <qwidget.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kgpg.h"
#include "keyservers.h"


static const char *description =
	I18N_NOOP("Kgpg - simple gui for gpg\n\nKgpg was designed to make gpg very easy to use.\nI tried to make it as secure as possible.\nHope you enjoy it...");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE

static KCmdLineOptions options[] =
{
{ "e", I18N_NOOP("encrypt file"), 0 },
{ "c", I18N_NOOP("decrypt clipboard & open editor"), 0 },
{"k", I18N_NOOP("open key manager"), 0 },
{"K", I18N_NOOP("open keyserver dialog"), 0 },
{ "d", I18N_NOOP("decrypt file"), 0 },
{ "s", I18N_NOOP("show encrypted file"), 0 },
{ "S", I18N_NOOP("sign file"), 0 },
{ "V", I18N_NOOP("verify signature"), 0 },
{ "C", I18N_NOOP("encrypt clipboard & copy resulting text in clipboard"), 0 },
{ "+file", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0}
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};


int main(int argc, char *argv[])
{

	KAboutData aboutData( "kgpg", "kgpg",
                              VERSION, description, KAboutData::License_GPL,
                              "(c) 2002, y0k0", 0, 0, "bj@altern.org");
	aboutData.addAuthor("y0k0",0, "bj@altern.org");
	aboutData.addCredit("Christoph Thielecke",I18N_NOOP("German translation"),"crissi99@gmx.de");
	aboutData.addCredit("Daniele Medri",I18N_NOOP("Italian translation"),"madrid@linuxmeeting.net");

KCmdLineArgs::init( argc, argv, &aboutData );
KCmdLineArgs::addCmdLineOptions( options );  
KApplication::addCmdLineOptions();
  
  KApplication app;    
   
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString opmode="";
  KURL FileToOpen=0;
  

 if (args->isSet("k")!=0)
 {
 listKeys *creat=new listKeys(0,i18n("Key Management"));
 creat->show();
 }
 else if (args->isSet("K")!=0)
 {
keyServer *ks=new keyServer(0);
ks->exec();
delete ks;
exit(1);
 }
 else
 {
 if (args->isSet("c")!=0)  opmode="clipboard";
 else
 if (args->isSet("C")!=0)  opmode="clipboardEnc";
 else if (args->count()>0)
  {
FileToOpen=args->url(0); 
opmode="decrypt";
 if (args->isSet("e")!=0)  opmode="encrypt";
 else if (args->isSet("s")!=0)  opmode="show";
 else if (args->isSet("V")!=0)  opmode="verify";
 else if (args->isSet("S")!=0)  opmode="sign";
 else if (FileToOpen.filename().endsWith(".sig")) opmode="verify";
 }
 KgpgApp *kgpg = new KgpgApp("kgpg",FileToOpen,opmode);
  
 if ((opmode=="") || (opmode=="show") || (opmode=="clipboard")) kgpg->show(); 
}
 return app.exec();
}
