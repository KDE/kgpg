/***************************************************************************
                          kgpg.h  -  description
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

#ifndef KGPG_H
#define KGPG_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt


// include files for KDE
#include <kapp.h>
#include <kmainwindow.h>
#include <kaction.h>
#include <kurl.h>
#include <kpassivepopup.h>


#include "listkeys.h"
#include "kgpgview.h"
#include "popuppublic.h"
#include "popupname.h"
#include "kgpgoptions.h"

// forward declaration of the Kgpg classes
//class KgpgView;
//class popupPublic;
//class KgpgOptions;

/**
  * The base class for Kgpg application windows. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. An instance of KgpgView creates your center view, which is connected
  * to the window's Doc object.
  * KgpgApp reimplements the methods that KMainWindow provides for main window handling and supports
  * full session management as well as using KActions.
  * @see KMainWindow
  * @see KApplication
  * @see KConfig
  *
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team.
  * @version KDevelop version 1.2 code generation
  */

class KgpgApp : public KMainWindow
  {
    Q_OBJECT

    friend class KgpgView;

public:
    /** construtor of KgpgApp, calls all init functions to create the application.
     */
    KgpgApp(const char* name=0,KURL fileToOpen=0,QString opmode=0);
    ~KgpgApp();
    /** opens a file specified by commandline option
     */
    void openDocumentFile(const KURL& url=0);
    void openEncryptedDocumentFile(const KURL& url=0,QString userIDs="");
    /** returns a pointer to the current document connected to the KTMainWindow instance and is used by
     * the View class to access the document object's methods
     */
    KURL Docname;
    int version;
    bool ascii,untrusted,hideid,pgpcomp,fastact,encrypttodefault,encryptfileto,tipofday;//,edecrypt;
	bool commandLineMode;
    QString messages,defaultkey,filekey;
    QPixmap fileEnc,fileDec;
        KgpgView *view;


protected:
    /** save general Options like all bar positions and status as well as the geometry and the recent file list to the configuration
     * file
     */

    /** read general Options again and initialize all variables like the recent file list
     */
    void readOptions(bool doresize=true);
    /** initializes the KActions of the application */
    void initActions();
    /** sets up the statusbar for the main window by initialzing a statuslabel.
     */
    void initStatusBar();
    /** initializes the document object of the main window that is connected to the view in initView().
     * @see initView();
     */
    //    void initDocument();
    /** creates the centerwidget of the KTMainWindow instance and sets it as the view
     */
    void initView();

private slots:
void killDisplayClip();
void expressQuit();
void encryptClipboard(QString &selec,QString);
void fastencode(QString &selec,QString encryptOptions,bool shred,bool symetric);
void fastdecode(bool quit);
void slotFileEnc();
void slotman();
void slotFileDec();
void slotExpressQuit();
//public slots:
void processenc(bool res);
void processdec(bool res);
void processdec2(bool res);
void processdecover(bool res);
void shredprocessenc(bool res);
void slotTip();
void firstrun();
void saveOptions();
void slotCheckMd5();
void slotPreSignFile();
void slotSignFile(KURL url);
void slotClip();
void slotVerifyFile(KURL url);
void slotPreVerifyFile();
void slotOptions();
    void slotundo();
    void slotredo();

    void slotManageKey();
    /** open a new application window by creating a new instance of KgpgApp */
    /** clears the document in the actual view to reuse it as the new document */
    void slotFileNew();
    /** open a file and load it into the document*/
    void slotFileOpen();
    /** opens a file from the recent files menu */
    /** save a document */
    void slotFileSave();
    /** save a document by a new filename*/
    void slotFileSaveAs();
    /** print the actual file */
    void slotFilePrint();
    /** closes all open windows by calling close() on each memberList item until the list is empty, then quits the application.
     * If queryClose() returns false because the user canceled the saveModified() dialog, the closing breaks.
     */
    void slotFileQuit();
    /** put the marked text/object into the clipboard and remove
     *	it from the document
     */
    void slotEditCut();
    /** put the marked text/object into the clipboard
     */
    void slotEditCopy();
    /** paste the clipboard into the document
     */
    void slotEditPaste();
    /** toggles the toolbar
     */

private:
QString decpassuid;
KURL decpasssrc,decpassdest;
QDialog *clippop;
KPassivePopup *pop;

KURL urlselected;
    /** the configuration object of the application */
    KConfig *config;
    /** view is the main widget which represents your working area. The View
     * class should handle all events of the view widget.  It is kept empty so
     * you can create your view according to your application's needs by
     * changing the view class.
     */

    //KgpgOptions *opts;

    /** doc represents your actual document and is created only once. It keeps
     * information such as filename and does the serialization of your files.
     */

    // KAction pointers to enable/disable actions
    KAction* manpage;
    KAction* fileSave;
    KAction* fileEncrypt;
    KAction* fileDecrypt;
    KAction* editUndo;
    KAction *editRedo,*helptips,*keysManage;
    KAction *signGenerate, *signVerify, *signCheck;

  //  KToggleAction* viewEditorFirst;
  };



#endif // KGPG_H
