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

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kgpg.h"


static const char *description =
	I18N_NOOP("Kgpg - simple gui for gpg\n\nKgpg was designed to make gpg very easy to use.\nI tried to make it as secure as possible.\nHope you enjoy it...");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE

static KCmdLineOptions options[] =
{
{ "e", I18N_NOOP("encrypt file"), 0 },
{ "c", I18N_NOOP("decrypt clipboard"), 0 },
{"k", I18N_NOOP("key manager"), 0 },
{ "d", I18N_NOOP("decrypt file"), 0 },
{ "s", I18N_NOOP("show encrypted file"), 0 },
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

KCmdLineArgs::init( argc, argv, &aboutData );
KCmdLineArgs::addCmdLineOptions( options );  
KApplication::addCmdLineOptions();

  KApplication app(argc, argv);  
  
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  
  

  
  //KUniqueApplication app;
  QString opmode="";
  KURL FileToOpen=0;
  
 if (args->isSet("c")!=0)  opmode="clipboard";
 else 
 if (args->isSet("k")!=0)
 {
 listKeys *creat=new listKeys(0,i18n("Key Management"));
 creat->show();
 }
 else
 {
 if (args->count()>0)
  {
FileToOpen=args->url(0); 
 if (args->isSet("e")!=0)  opmode="encrypt";
 else if (args->isSet("s")!=0)  opmode="show";
 else opmode="decrypt";
 }
 //KgpgApp *kgpg = new KgpgApp(0,"kgpg",FileToOpen,encmode,decmode,clipmode);
 KgpgApp *kgpg = new KgpgApp(0,"kgpg",FileToOpen,opmode);
 if ((opmode!="encrypt") && (opmode!="decrypt")) kgpg->show();
 
 
 /* if (app.isRestored())
  {
    RESTORE(KgpgApp);
  }
  else 
  {
    KgpgApp *kgpg = new KgpgApp();
    kgpg->show();
  }
*/
}
return app.exec();
}  
