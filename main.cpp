/*
 * Copyright (C) 2003 bj <bj@altern.org>
 */


#include <kuniqueapplication.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include "kgpg.h"

static const char *description =
	I18N_NOOP("Kgpg - simple gui for gpg\n\nKgpg was designed to make gpg very easy to use.\nI tried to make it as secure as possible.\nHope you enjoy it.");

static const char *version = "0.9.5";

static KCmdLineOptions options[] =
{
{ "e", I18N_NOOP("Encrypt file"), 0 },
{ "s", I18N_NOOP("Show encrypted file"), 0 },
{ "S", I18N_NOOP("Sign file"), 0 },
{ "V", I18N_NOOP("Verify signature"), 0 },
{ "+file", I18N_NOOP("File to open"), 0 },
    { 0, 0, 0}
  // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char **argv)
{

    KAboutData about("kgpg", I18N_NOOP("kgpg"), version, description,
                     KAboutData::License_GPL, "(C) 2003 bj", 0, 0, "bj@altern.org");
    about.addAuthor( "bj", 0, "bj@altern.org" );
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions(options);
	KUniqueApplication::addCmdLineOptions();
   
	 if (!KgpgAppletApp::start())
       return 0;

   KgpgAppletApp app;
   return app.exec();
	
	/* 
	  
	KUniqueApplication app;

    // register ourselves as a dcop client
  //app.dcopClient()->registerAs(app.name(), false);
    
  
  kgpgapplet widget;// = new kgpgapplet("kgpg");
  widget.show();
  return app.exec();*/
}
