/***************************************************************************
                          kgpgeditor.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
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

#ifndef KGPGEDITOR_H
#define KGPGEDITOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kmainwindow.h>

#include <qstringlist.h>
#include <kurl.h>

class KPassivePopup;
class KComboBox;
class KToggleAction;
class KAction;
class KgpgView;

class KgpgApp : public KMainWindow
{
        Q_OBJECT

        friend class KgpgView;

public:
        /** construtor of KgpgApp, calls all init functions to create the application.
         */
        KgpgApp(QWidget *parent=0, const char *name=0,WFlags f = 0,KShortcut goHome=QKeySequence(CTRL+Qt::Key_Home),bool mainWindow=false);
        ~KgpgApp();
        /** opens a file specified by commandline option
         */
        void openDocumentFile(const KURL& url,QString encoding=QString::null);
        void openEncryptedDocumentFile(const KURL& url);
        /** returns a pointer to the current document connected to the KTMainWindow instance and is used by
         * the View class to access the document object's methods
         */
        KURL Docname;
        int version;
        QString messages;
        KgpgView *view;
	KShortcut goDefaultKey;


protected:
        void readOptions(bool doresize=true);
        void saveOptions();
        void initActions();
        void initView();
        void closeEvent( QCloseEvent * e );

private slots:
        //void slotOptions();
        void slotFileQuit();
        void slotFileNew();
	void slotResetEncoding(bool enc);
        void slotFilePreEnc();
        void slotFilePreDec();
        void slotFileOpen();
        void slotFileSave();
        void slotFileSaveAs();
        void slotFilePrint();
        void slotEditCut();
        void slotEditCopy();
        void slotEditPaste();
        void slotSelectAll();
	void slotCheckMd5();

        void slotPreSignFile();
        void slotSignFile(KURL url);
        void slotVerifyFile(KURL url);
        void slotPreVerifyFile();
        void importSignatureKey(QString ID);

        void slotundo();
        void slotredo();
	void slotSetCharset();
	bool checkEncoding(QTextCodec *codec);
	void slotOptions();
	void slotKeyManager();


public slots:
	void slotSetFont(QFont myFont);
	void closeWindow();


private:

        QStringList customDecrypt;
	//KToggleAction *encodingAction ;
        KURL urlselected;
        KAction* fileSave, *editUndo, *editRedo;
	KComboBox *fontCombo;
	bool isMainWindow;
	QString textEncoding;

signals:

void refreshImported(QStringList);
void openChangeFont();
void openConfigDialog();
void encryptFiles(KURL::List fileList);
};

#endif // KGPGEDITOR_H

