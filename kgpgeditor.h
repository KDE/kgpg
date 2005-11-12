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

#ifndef __KGPGEDITOR_H__
#define __KGPGEDITOR_H__

#include <QStringList>

#include <kmainwindow.h>
#include <kurl.h>

class QCloseEvent;

class KPassivePopup;
class KToggleAction;
class KComboBox;
class KAction;
class KgpgView;

class KgpgEditor : public KMainWindow
{
    Q_OBJECT
    friend class KgpgView;

public:
    KgpgEditor(QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0, KShortcut goHome = QKeySequence(Qt::CTRL + Qt::Key_Home), bool mainWindow = false);
    ~KgpgEditor();

    void openDocumentFile(const KURL& url, QString encoding = QString::null);
    void openEncryptedDocumentFile(const KURL& url);

    KgpgView *view;

signals:
    void refreshImported(QStringList);
    void openChangeFont();
    void openConfigDialog();
    void encryptFiles(KURL::List fileList);

public slots:
    void slotSetFont(QFont myFont);
    void closeWindow();

protected:
    void readOptions(bool doresize = true);
    void saveOptions();
    void initActions();
    void initView();
    void closeEvent(QCloseEvent * e);

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

private:
    QStringList customDecrypt;
    QString textEncoding;
    QString messages;

    KToggleAction *encodingAction ;
    KURL urlselected;
    KAction *fileSave;
    KAction *editUndo;
    KAction *editRedo;
    KComboBox *fontCombo;
    KShortcut goDefaultKey;
    KURL Docname;

    bool isMainWindow;
    int version;
};

#endif // __KGPGEDITOR_H__
