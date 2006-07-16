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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QStringList>
#include <QClipboard>
#include <QLabel>

#include <kuniqueapplication.h>
#include <ksystemtray.h>
#include <kshortcut.h>
#include <kurl.h>

class QDragEnterEvent;
class QDropEvent;
class QMenu;

class KPassivePopup;
class KgpgWizard;
class KAboutData;
class KTempFile;
class KAction;

class KgpgSelectPublicKeyDlg;
class KgpgInterface;
class KeysManager;

class MyView : public QLabel
{
    Q_OBJECT

public:
    MyView(QWidget *parent = 0, const char *name = 0);
    ~MyView();

    KUrl droppedUrl;
    KUrl::List droppedUrls;
    KTempFile *kgpgfoldertmp;
    KShortcut goDefaultKey;

signals:
    void setFont(QFont);
    void readAgain2();
    void createNewKey();
    void updateDefault(QString);
    void importedKeys(QStringList);

public slots:
    /**
     * When you clic on "encrypt the clipboard" in the systray,
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
    void shredDroppedFile();

    void busyMessage(QString mssge, bool reset = false);
    void slotVerifyFile();
    void encryptDroppedFolder();
    void startFolderEncode(QStringList selec, QStringList encryptOptions, bool, bool symetric);
    void slotFolderFinished(KUrl, KgpgInterface*);
    void slotFolderFinishedError(QString errmsge, KgpgInterface*);
    void encryptFiles(KUrl::List urls);
    void installShred();

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
    KAboutData *_aboutData;
    QStringList customDecrypt;
    KgpgWizard *wiz;
    KPassivePopup *pop;
    KTempFile *kgpgFolderExtract;
    int compressionScheme;
    int openTasks;
    KgpgSelectPublicKeyDlg *dialogue;
    QClipboard::Mode clipboardMode;

private slots:
    void slotWizardClose();
    void startWizard();
    void slotWizardChange();
    void slotSaveOptionsPath();
    void slotGenKey();
    void importSignature(QString ID);
    void slotSetClip(QString newtxt);
    void slotPassiveClip();
    void encryptClipboard(QStringList selec, QStringList encryptOptions, bool, bool symmetric);
    void help();
    void about();
    void firstRun();
    void readOptions();
    void droppedfile(KUrl::List);
    void droppedtext(QString inputText, bool allowEncrypt = true);
    void unArchive();
    void slotSetCompression(int cp);
    void decryptNextFile();
};

class kgpgapplet : public KSystemTray
{
    Q_OBJECT

public:
    kgpgapplet(QWidget *parent = 0);
    ~kgpgapplet();
    MyView *w;

private:
    KSystemTray *kgpgapp;

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
    void wizardOver(QString defaultKeyId);
};

#endif // KGPGAPPLET_H
