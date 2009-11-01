/***************************************************************************
 *   Copyright 2002 by Jean-Baptiste Mardelle <bj@altern.org>              *
 *   Copyright 2008,2009 by Rolf Eike Beer <kde@opensource.sf-tec.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KGPGEXTERNALACTIONS_H
#define _KGPGEXTERNALACTIONS_H

#include <QObject>
#include <QClipboard>
#include <QStringList>

#include <KShortcut>
#include <KUrl>

class KeysManager;
class KGpgFirstAssistant;
class KGpgItemModel;
class KgpgLibrary;
class KgpgSelectPublicKeyDlg;
class KGpgTextInterface;
class KPassivePopup;
class KTemporaryFile;
class QFont;
class QString;

/**
 * @brief handle actions called from outside the application
 *
 * This class takes care about everything sent to us from outside the
 * application, e.g. command line arguments given on startup.
 */
class KGpgExternalActions : public QObject
{
	Q_OBJECT

public:
	KGpgExternalActions(KeysManager *parent, KGpgItemModel *model);
	~KGpgExternalActions();

	KUrl droppedUrl;
	KUrl::List droppedUrls;
	KTemporaryFile *kgpgfoldertmp;
	KShortcut goDefaultKey;

signals:
	void readAgain2();
	void createNewKey();
	void updateDefault(QString);
	void importDrop(const QString &);

public slots:
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

private:
	QStringList customDecrypt;
	KGpgFirstAssistant *m_assistant;
	KPassivePopup *pop;
	KTemporaryFile *kgpgFolderExtract;
	int compressionScheme;
	int openTasks;
	KgpgSelectPublicKeyDlg *dialog;
	QClipboard::Mode clipboardMode;
	KGpgItemModel *m_model;

	void startAssistant();
	void firstRun();

	KUrl::List m_decryptionFailed;
	QWidget *m_parentWidget;
	KeysManager *m_keysmanager;

private slots:
	void slotAssistantClose();
	void slotSaveOptionsPath();
	void importSignature(const QString &ID);
	void help();
	void readOptions();
	void slotSetCompression(int cp);
	void decryptNextFile(KgpgLibrary *lib, const KUrl &failed);
	void decryptFile(KgpgLibrary *lib);
};

#endif /* _KGPGEXTERNALACTIONS_H */
