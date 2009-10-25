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

#include <KXmlGuiWindow>
#include <KUrl>

class KToggleAction;
class KAction;
class KFind;

class KgpgInterface;
class KgpgView;
class KGpgItemModel;
class KgpgLibrary;

class KgpgEditor : public KXmlGuiWindow
{
    Q_OBJECT
    friend class KgpgView;

    KgpgEditor();	// = delete C++0x
public:
    KgpgEditor(QWidget *parent, KGpgItemModel *model, Qt::WFlags f);
    ~KgpgEditor();

    void openDocumentFile(const KUrl& url, const QString &encoding = QString());
    void openEncryptedDocumentFile(const KUrl& url);

    KgpgView *view;

signals:
    void refreshImported(QStringList);
    void openChangeFont();
    void openConfigDialog();
    void encryptFiles(KUrl::List fileList);

public slots:
    void slotSetFont(QFont myFont);
    void closeWindow();

protected:
    void saveOptions();
    void initActions();
    bool queryClose();
    bool saveBeforeClear();

private slots:
    // File menu
    void slotFileNew();
    void slotFileOpen();
    bool slotFileSave();
    bool slotFileSaveAs();
    void slotFilePrint();
    void slotFilePreEnc();
    void slotFilePreDec();
    void slotKeyManager();
    void slotFileClose();

    // Edit menu
    void slotundo();
    void slotredo();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotSelectAll();
    void slotFind();
    void slotFindNext();
    void slotFindPrev();
    void slotFindText();

    // Coding menu
    void slotSetCharset();
    void slotResetEncoding(bool enc);
    bool checkEncoding(QTextCodec *codec);

    // Signing menu
    void slotPreSignFile();
    void slotSignFile(const KUrl &url);
    void slotSignFileFin(KgpgInterface *interface);
    void slotPreVerifyFile();
    void slotVerifyFile(const KUrl &url);
    void slotCheckMd5();
    void importSignatureKey(const QString &id);

    // Options menu
    void slotOptions();

    void slotUndoAvailable(const bool &v);
    void slotRedoAvailable(const bool &v);
    void slotCopyAvailable(const bool &v);

    void modified();
    void newText();

    void slotRefreshImported(KgpgLibrary *lib, const QStringList &keys);

private:
    QStringList m_customdecrypt;
    QString m_textencoding;

    KToggleAction *m_encodingaction;
    KAction *m_editundo;
    KAction *m_editredo;
    KAction *m_editcopy;
    KAction *m_editcut;
    KFind *m_find;
    KUrl m_docname;

    bool m_textchanged;		//< text was changed since last save
    bool m_emptytext;		//< this was not saved to a file ever

    KGpgItemModel *m_model;
};

#endif // KGPGEDITOR_H
