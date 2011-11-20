/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2008,2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KGPGEXTERNALACTIONS_H
#define _KGPGEXTERNALACTIONS_H

#include <QObject>
#include <QClipboard>
#include <QPointer>
#include <QStringList>

#include <KUrl>

class KeysManager;
class KGpgFirstAssistant;
class KGpgItemModel;
class KgpgLibrary;
class KgpgSelectPublicKeyDlg;
class KGpgTextInterface;
class KPassivePopup;
class KShortcut;
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

	void busyMessage(const QString &mssge);
	void slotVerifyFile();
	void encryptDroppedFolder();
	void startFolderEncode();
	void slotFolderFinished(const KUrl &);
	void slotFolderFinishedError(const QString &errmsge);
	void encryptFiles(KUrl::List urls);
	void slotAbortEnc();

private:
	QStringList customDecrypt;
	QPointer<KGpgFirstAssistant> m_assistant;
	KPassivePopup *pop;
	KTemporaryFile *kgpgFolderExtract;
	int compressionScheme;
	KgpgSelectPublicKeyDlg *dialog;
	QClipboard::Mode clipboardMode;
	KGpgItemModel *m_model;

	void startAssistant();
	void firstRun();

	KUrl::List m_decryptionFailed;
	QWidget *m_parentWidget;
	KeysManager *m_keysmanager;

	KShortcut goDefaultKey() const;

private slots:
	void slotSaveOptionsPath();
	void importSignature(const QString &ID);
	void help();
	void readOptions();
	void slotSetCompression(int cp);
	void slotDecryptionDone(int status);
	void decryptFile();
};

#endif /* _KGPGEXTERNALACTIONS_H */
