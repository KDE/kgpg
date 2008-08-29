/***************************************************************************
                          kgpg.h  -  description
                             -------------------
    begin                : Mon Nov 18 2002
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

#ifndef KGPGAPPLET_H
#define KGPGAPPLET_H

#include <QClipboard>

#include <KUniqueApplication>
#include <KSystemTrayIcon>
#include <KShortcut>
#include <KUrl>

class QDragEnterEvent;
class QDropEvent;
class QMenu;

class KPassivePopup;
class KGpgFirstAssistant;
class KAboutData;
class KTemporaryFile;
class KAction;

class KgpgSelectPublicKeyDlg;
class KGpgTextInterface;
class KeysManager;
class KgpgLibrary;
class KGpgItemModel;
class KgpgInterface;

class KGpgUserActions : public QObject
{
    Q_OBJECT

public:
    KGpgUserActions(QWidget *parent, KSystemTrayIcon *parentTrayIcon, KGpgItemModel *model);
    ~KGpgUserActions();

    KUrl droppedUrl;
    KUrl::List droppedUrls;
    KTemporaryFile *kgpgfoldertmp;
    KShortcut goDefaultKey;

signals:
    void setFont(QFont);
    void readAgain2();
    void createNewKey();
    void updateDefault(QString);

public slots:
    /**
     * When you click on "encrypt the clipboard" in the systray,
     * this slot will open the dialog to choose a key and encrypt the
     * clipboard.
     */
    void clipEncrypt();
    void clipDecrypt();
    void clipSign(bool openEditor = true);

    void encryptDroppedFile();
    void decryptDroppedFile();
    void signDroppedFile();
    void showDroppedFile();

    void busyMessage(const QString &mssge, bool reset = false);
    void slotVerifyFile();
    void encryptDroppedFolder();
    void startFolderEncode();
    void slotFolderFinished(const KUrl &, KGpgTextInterface *);
    void slotFolderFinishedError(const QString &errmsge, KGpgTextInterface *);
    void encryptFiles(KUrl::List urls);
    void slotAbortEnc();

protected:
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent*);

private:
    KAction *saveDecrypt;
    KAction *showDecrypt;
    KAction *encrypt;
    KAction *sign;

    QMenu *droppopup;
    QMenu *udroppopup;
    QStringList customDecrypt;
    KGpgFirstAssistant *m_assistant;
    KPassivePopup *pop;
    KTemporaryFile *kgpgFolderExtract;
    int compressionScheme;
    int openTasks;
    KgpgSelectPublicKeyDlg *dialog;
    QClipboard::Mode clipboardMode;
    KSystemTrayIcon *trayIcon;
    KGpgItemModel *m_model;

    void startAssistant();
    void firstRun();

    KUrl::List m_decryptionFailed;
    QWidget *m_parentWidget;

private slots:
    void slotAssistantClose();
    void slotSaveOptionsPath();
    void importSignature(const QString &ID);
    void slotSetClip(const QString &newtxt);
    void slotPassiveClip();
    void encryptClipboard(QStringList selec, QStringList encryptOptions, const bool symmetric);
    void help();
    void about();
    void readOptions();
    void droppedfile(KUrl::List);
    void droppedtext(const QString &inputText, bool allowEncrypt = true);
    void slotSetCompression(int cp);
    void decryptNextFile(KgpgLibrary *lib, const KUrl &failed);
    void decryptFile(KgpgLibrary *lib);
    void slotImportedKeys(KgpgInterface *iface, const QStringList &);
    void slotImportedKeys(KgpgLibrary *lib, const QStringList &);
};

class kgpgapplet : public KSystemTrayIcon
{
    Q_OBJECT

public:
    kgpgapplet(QWidget *parent, KGpgItemModel *model);
    ~kgpgapplet();
    KGpgUserActions *w;

private:
    KSystemTrayIcon *kgpgapp;

private slots:
    void slotOpenKeyManager();
    void slotOpenServerDialog();
    void showOptions();
};

class KCmdLineArgs;

class KgpgAppletApp : public KUniqueApplication
{
    Q_OBJECT
    friend class kgpgapplet;

public:
    KgpgAppletApp();
    ~KgpgAppletApp();

    int newInstance ();
    bool running;
    KUrl::List urlList;
    KShortcut goHome;

protected:
    KCmdLineArgs *args;

private:
    kgpgapplet *kgpg_applet;
    KeysManager *s_keyManager;

private slots:
    void slotHandleQuit();
    void assistantOver(const QString &defaultKeyId);
};

#endif // KGPGAPPLET_H
