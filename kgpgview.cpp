/***************************************************************************
                          kgpgview.cpp  -  description
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

//////////////////////////////////////////////////////////////  code for the main view (editor)

// include files for Qt
//#include <qprinter.h>

// application specific includes

#include <qscrollview.h>
#include <qregexp.h>

#include <kio/netaccess.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kurldrag.h>

#include "kgpginterface.h"
#include "popupname.h"
#include "kgpgview.h"
#include "kgpg.h"


//////////////// configuration for editor

MyEditor::MyEditor( QWidget *parent, const char *name )
                : KTextEdit( parent, name )
{
        setTextFormat(PlainText);
	setCheckSpellingEnabled (true);
        setAcceptDrops(true);
}

void MyEditor::contentsDragEnterEvent( QDragEnterEvent *e )
{
        ////////////////   if a file is dragged into editor ...
        e->accept (KURLDrag::canDecode(e) || QTextDrag::canDecode (e));
        //e->accept (QTextDrag::canDecode (e));
}




void MyEditor::contentsDropEvent( QDropEvent *e )
{
        /////////////////    decode dropped file
        KURL::List list;
        QString text;
        if ( KURLDrag::decode( e, list ) )
                slotDroppedFile(list.first());
        else if ( QTextDrag::decode(e, text) )
                insert(text);
}

void MyEditor::slotDroppedFile(KURL url)
{
        /////////////////    decide what to do with dropped file
        QString text;
        if (!tempFile.isEmpty()) {
                KIO::NetAccess::removeTempFile(tempFile);
                tempFile=QString::null;
        }

        if (url.isLocalFile())
                tempFile = url.path();
        else {
                if (KMessageBox::warningContinueCancel(0,i18n("<qt><b>Remote file dropped</b>.<br>The remote file will now be copied to a temporary file to process requested operation. This temporary file will be deleted after operation.</qt>"),0,KStdGuiItem::cont(),"RemoteFileWarning")!=KMessageBox::Continue)
                        return;
                if (!KIO::NetAccess::download (url, tempFile)) {
                        KMessageBox::sorry(this,i18n("Could not download file."));
                        return;
                }
        }

        
                /////////////  if dropped filename ends with gpg, pgp or asc, try to decode it
                if ((tempFile.endsWith(".gpg")) || (tempFile.endsWith(".asc")) || (tempFile.endsWith(".pgp"))) {
                        slotDecodeFile(tempFile);
                }
		else slotCheckContent(tempFile);
}


void MyEditor::slotCheckContent(QString fileToCheck, bool checkForPgpMessage)
{
QFile qfile(fileToCheck);
        if (qfile.open(IO_ReadOnly)) {
                //////////   open file
                
                        QTextStream t( &qfile );
                        QString result(t.read());
                        //////////////     if  pgp data found, decode it
                        if ((checkForPgpMessage) && (result.startsWith("-----BEGIN PGP MESSAGE"))) {
                                qfile.close();
                                slotDecodeFile(fileToCheck);
                                return;
                        } else
                                if (result.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK")) {//////  dropped file is a public key, ask for import
                                        qfile.close();
                                        int result=KMessageBox::warningContinueCancel(this,i18n("<p>The file <b>%1</b> is a public key.<br>Do you want to import it ?</p>").arg(fileToCheck),i18n("Warning"));
                                        if (result==KMessageBox::Cancel) {
                                                KIO::NetAccess::removeTempFile(fileToCheck);
                                                return;
                                        } else {
                                                KgpgInterface *importKeyProcess=new KgpgInterface();
                                                importKeyProcess->importKeyURL(KURL(fileToCheck));
                                                connect(importKeyProcess,SIGNAL(importfinished(QStringList)),this,SLOT(slotProcessResult(QStringList)));
                                                return;
                                        }
                                } else {
                                        if (result.startsWith("-----BEGIN PGP PRIVATE KEY BLOCK")) {
						qfile.close();
                                                KMessageBox::information(0,i18n("This file is a private key!\nPlease use kgpg key management to import it."));
                                                KIO::NetAccess::removeTempFile(fileToCheck);
                                                return;
                                        }

                                        setText(result);
                                        qfile.close();
                                        KIO::NetAccess::removeTempFile(fileToCheck);
                                }
                }
}


void MyEditor::editorUpdateDecryptedtxt(QString newtxt)
{
	setText(newtxt);
}

void MyEditor::editorFailedDecryptedtxt(QString newtxt)
{
	KMessageBox::detailedSorry(this,i18n("Decryption failed."),newtxt);
}


void MyEditor::slotDecodeFile(QString fname)
{
        ////////////////     decode file from given url into editor
        QFile qfile(QFile::encodeName(fname));
        if (qfile.open(IO_ReadOnly)) {			
	KConfig *ksConfig=kapp->config();
	ksConfig->setGroup("Decryption");
	KgpgInterface *txtDecrypt=new KgpgInterface();
        connect (txtDecrypt,SIGNAL(txtdecryptionfinished(QString)),this,SLOT(editorUpdateDecryptedtxt(QString)));
	connect (txtDecrypt,SIGNAL(txtdecryptionfailed(QString)),this,SLOT(editorFailedDecryptedtxt(QString)));
        txtDecrypt->KgpgDecryptFileToText(KURL(fname),QStringList::split(QString(" "),ksConfig->readEntry("custom_decrypt").simplifyWhiteSpace()));
        } else
                KMessageBox::sorry(this,i18n("Unable to read file."));
}


void MyEditor::slotProcessResult(QStringList iKeys)
{
	emit refreshImported(iKeys);
        KIO::NetAccess::removeTempFile(tempFile);
        tempFile=QString::null;
}


////////////////////////// main view configuration

KgpgView::KgpgView(QWidget *parent, const char *name) : QWidget(parent, name)
{
        viewreadopts();
        editor=new MyEditor(this);

        /////    layout

        QVBoxLayout *vbox=new QVBoxLayout(this,3);

        editor->setReadOnly( false );
        editor->setUndoRedoEnabled(true);
        editor->setUndoDepth(5);

        setAcceptDrops(true);

        KButtonBox *boutonbox=new KButtonBox(this,KButtonBox::Horizontal,15,12);
        boutonbox->addStretch(1);

        bouton0=boutonbox->addButton(i18n("S&ign/Verify"),this,SLOT(clearSign()),TRUE);
        bouton1=boutonbox->addButton(i18n("En&crypt"),this,SLOT(popuppublic()),TRUE);
        bouton2=boutonbox->addButton(i18n("&Decrypt"),this,SLOT(slotdecode()),TRUE);

        QObject::connect(editor,SIGNAL(textChanged()),this,SLOT(modified()));

        boutonbox->layout();
        editor->resize(editor->maximumSize());
        vbox->addWidget(editor);
        vbox->addWidget(boutonbox);
}


void KgpgView::modified()
{
        /////////////// notify for changes in editor window
        KgpgApp *win=(KgpgApp *) parent();
        if (win->fileSave->isEnabled()==false) {
                QString capt=win->Docname.filename();
                if (capt.isEmpty())
                        capt=i18n("untitled");
                win->setCaption(capt,true);
                win->fileSave->setEnabled(true);
                win->editUndo->setEnabled(true);
        }

}

void KgpgView::viewreadopts()
{
        ////////  read default options for encryption

        KgpgApp *win=(KgpgApp *) parent();
        pubascii=win->ascii;
        pubpgp=win->pgpcomp;
        pubuntrusted=win->untrusted;
}

void KgpgView::clearSign()
{
        QString mess=editor->text();
        if (mess.startsWith("-----BEGIN PGP SIGNED")) {
                //////////////////////   this is a signed message, verify it

                ///////////////////  generate gpg command

                ///////////////// run command
                FILE *fp,*cmdstatus;
                int process[2];

                pipe(process);
                cmdstatus = fdopen(process[1], "w");
                QString line="echo "+KShellProcess::quote(mess.utf8());
                line+=" | gpg --no-tty  --no-secmem-warning --status-fd="+QString::number(process[1])+" --verify";
                fp=popen(QFile::encodeName(line),"r");
                pclose(fp);
                fclose(cmdstatus);

                int Len;
                char Buff[500]="\0";
                QString verifyResult,lineRead;

                //////////////////////////   read gpg output
                while (read(process[0], &Len, sizeof(Len)) > 0) {
                        read(process[0],Buff, Len);
                        lineRead=Buff;
                        if (lineRead.find("GOODSIG",0,FALSE)!=-1)
                                lineRead.remove(0,lineRead.find("GOODSIG",0,FALSE)+7);
                        if (lineRead.find("BADSIG",0,FALSE)!=-1)
                                lineRead.remove(0,lineRead.find("BADSIG",0,FALSE)+6);
                        if (lineRead.find("NO_PUBKEY",0,FALSE)!=-1)
                                lineRead.remove(0,lineRead.find("NO_PUBKEY",0,FALSE)+9);
                        verifyResult+=Buff;
                }
                if ((verifyResult.find("GOODSIG",0,FALSE)!=-1) && (verifyResult.find("BADSIG",0,FALSE)==-1)) {
                        lineRead=lineRead.left(lineRead.find("\n",0,FALSE));
                        lineRead=lineRead.stripWhiteSpace();
                        QString resultKey=lineRead.section(" ",1,-1);
                        QString resultID=lineRead.section(" ",0,0);
                        KMessageBox::information(this,i18n("<qt>Good signature from:<br><b>%1</b><br>Key ID: %2</qt>").arg(resultKey.replace(QRegExp("<"),"&lt;")).arg(resultID));
                } else
                        if ((verifyResult.find("NO_PUBKEY",0,FALSE)!=-1) && (verifyResult.find("BADSIG",0,FALSE)==-1)) {
                                lineRead=lineRead.left(lineRead.find("\n",0,FALSE));
                                lineRead=lineRead.stripWhiteSpace();

                                if (KMessageBox::questionYesNo(0,i18n("<qt><b>Missing signature:</b><br>Key id: %1<br><br>"
                                                                      "Do you want to import this key from a keyserver?</qt>").arg(lineRead),i18n("Missing Key"))==KMessageBox::Yes) {
                                        keyServer *kser=new keyServer(0,"server_dialog",false);
                                        kser->page->kLEimportid->setText(lineRead);
                                        kser->slotImport();
                                }
                                return;
                        } else {
                                lineRead=lineRead.left(lineRead.find("\n",0,FALSE));
                                lineRead=lineRead.stripWhiteSpace();
                                QString resultKey=lineRead.section(" ",1,-1);
                                QString resultID=lineRead.section(" ",0,0);
                                KMessageBox::sorry(this,i18n("<qt><b>BAD signature</b> from:<br>%1<br>Key ID: %2<br><br><b>Text is corrupted!</b></qt>").arg(resultKey.replace(QRegExp("<"),"&lt;")).arg(resultID));
                        }
        } else {
                /////    Sign the text in Editor


                QString signKeyID,signKeyMail;
                FILE *fp,*pass;
                int ppass[2];
                char buffer[200];
                QCString password;

                ///// open key selection dialog
                KgpgSelKey *opts=new KgpgSelKey(this,0);

                if (opts->exec()==QDialog::Accepted) {
                        signKeyID=opts->getkeyID();
                        signKeyMail=opts->getkeyMail();
                } else {
                        delete opts;
                        return;
                }
                delete opts;
                /////////////////////  get passphrase
		
		KConfig *ksConfig=kapp->config();
		ksConfig->setGroup("GPG Settings");
                bool useAgent=KgpgInterface::getGpgBoolSetting("use-agent",ksConfig->readPathEntry("gpg_config_path"));
		
                if (!getenv("GPG_AGENT_INFO") || !useAgent) {
                        int code=KPasswordDialog::getPassword(password,i18n("Enter passphrase for <b>%1</b>:").arg(signKeyMail.replace(QRegExp("<"),"&lt;")));
                        ///////////////////   ask for password
                        if (code!=QDialog::Accepted)
                                return;

                        ///////////////////   pipe passphrase
                        pipe(ppass);
                        pass = fdopen(ppass[1], "w");
                        fwrite(password, sizeof(char), strlen(password), pass);
                        //        fwrite("\n", sizeof(char), 1, pass);
                        fclose(pass);
                }
                ///////////////////  generate gpg command
                QString line="echo ";
                //mess=mess.replace(QRegExp("\\\\") , "\\\\").replace(QRegExp("\\\"") , "\\\"").replace(QRegExp("\\$") , "\\$");
                line+=KShellProcess::quote(mess.utf8());
                line+=" | gpg ";
                if (pubpgp)
                        line+="--pgp6 ";
                if (!getenv("GPG_AGENT_INFO") || !useAgent) {
                        line+="--passphrase-fd ";
                        QString fd;
                        fd.setNum(ppass[0]);
                        line+=fd;
                }
                line+=" --no-tty --clearsign -u ";
                line+=KShellProcess::quote(signKeyID);
                //KMessageBox::sorry(0,QString(line));
                QString tst=QString::null;

                ///////////////// run command
                fp = popen(QFile::encodeName(line), "r");
                while ( fgets( buffer, sizeof(buffer), fp))
                        tst+=buffer;
                pclose(fp);

                /////////////////  paste result into editor
                if (!tst.isEmpty()) {
                        editor->setText(QString::fromUtf8(tst.ascii()));
                        KgpgApp *win=(KgpgApp *) parent();
                        win->editRedo->setEnabled(false);
                        win->editUndo->setEnabled(false);
                } else
                        KMessageBox::sorry(this,i18n("Signing not possible: bad passphrase or missing key"));
        }
}

void KgpgView::popuppublic()
{
        /////    popup dialog to select public key for encryption

        ////////  open dialog --> popuppublic.cpp
        popupPublic *dialogue=new popupPublic(this, "public_keys", 0,false);
        connect(dialogue,SIGNAL(selectedKey(QStringList,QStringList,bool,bool)),this,SLOT(encodetxt(QStringList,QStringList)));
        dialogue->exec();
        delete dialogue;
}


//////////////////////////////////////////////////////////////////////////////////////     decode

void KgpgView::slotdecode()
{
        ///////////////    decode data from the editor. triggered by the decode button

        QString dests,encUsers;
        messages=QString::null;

        encUsers=KgpgInterface::extractKeyName(editor->text());

        if (encUsers.isEmpty())
                encUsers=i18n("[No user id found]");

        //QString resultat=KgpgInterface::KgpgDecryptText(editor->text(),encUsers);
	KConfig *ksConfig=kapp->config();
	ksConfig->setGroup("Decryption");
	KgpgInterface *txtDecrypt=new KgpgInterface();
        connect (txtDecrypt,SIGNAL(txtdecryptionfinished(QString)),this,SLOT(updateDecryptedtxt(QString)));
	connect (txtDecrypt,SIGNAL(txtdecryptionfailed(QString)),this,SLOT(failedDecryptedtxt(QString)));
        txtDecrypt->KgpgDecryptText(editor->text(),QStringList::split(QString(" "),ksConfig->readEntry("custom_decrypt").simplifyWhiteSpace()));
	
	/*
        KgpgApp *win=(KgpgApp *) parent();
        if (!resultat.isEmpty()) {
                editor->setText(resultat);
                win->editRedo->setEnabled(false);
                win->editUndo->setEnabled(false);
        }*/
}

void KgpgView::updateDecryptedtxt(QString newtxt)
{
	editor->setText(newtxt);
}

void KgpgView::failedDecryptedtxt(QString newtxt)
{
	KMessageBox::detailedSorry(this,i18n("Decryption failed."),newtxt);
}


void KgpgView::encodetxt(QStringList selec,QStringList encryptOptions)
{
        //////////////////              encode from editor
        if (pubpgp)
                encryptOptions<<"--pgp6";
        if (selec.isEmpty()) {
                KMessageBox::sorry(0,i18n("You have not chosen an encryption key."));
                return;
        }

        KgpgInterface *txtCrypt=new KgpgInterface();
        connect (txtCrypt,SIGNAL(txtencryptionfinished(QString)),this,SLOT(updatetxt(QString)));
        txtCrypt->KgpgEncryptText(editor->text(),selec,encryptOptions);
        //KMessageBox::sorry(0,"OVER");

        //KgpgInterface::KgpgEncryptText(editor->text(),selec,encryptOptions);
        //if (!resultat.isEmpty()) editor->setText(resultat);
        //else KMessageBox::sorry(this,i18n("Decryption failed."));
}

void KgpgView::updatetxt(QString newtxt)
{
        if (!newtxt.isEmpty())
                editor->setText(newtxt);
        else
                KMessageBox::sorry(this,i18n("Encryption failed."));
}


KgpgView::~KgpgView()
{}

/*
void KgpgView::print(QPrinter *pPrinter)
{
  QPainter printpainter;
  printpainter.begin(pPrinter);

  // TODO: add your printing code here

  printpainter.end();
}
*/
#include "kgpgview.moc"

