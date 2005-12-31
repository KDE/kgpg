/***************************************************************************
                          kgpglibrary.h  -  description
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

#ifndef __KGPGLIBRARY_H__
#define __KGPGLIBRARY_H__

#include <QObject>

#include <kshortcut.h>
#include <kurl.h>

class KPassivePopup;
class KIO::Job;

class KgpgInterface;

class KgpgLibrary : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    KgpgLibrary(QWidget *parent = 0, const bool &pgpExtension = false);

signals:
    void encryptionOver();
    void decryptionOver();
    void importOver(QStringList);
    void systemMessage(QString, bool reset = false);
    void photoAdded();

public slots:
    void slotFileEnc(const KURL::List &urls = KURL(""), const QStringList &opts = QStringList(), const QStringList &defaultKey = QStringList(), const KShortcut &goDefaultKey = QKeySequence(Qt::CTRL + Qt::Key_Home));
    void slotFileDec(const KURL &srcUrl, const KURL &destUrl, const QStringList &customDecryptOption = QStringList());
    void shredProcessEnc(const KURL::List &filesToShred);

    void addPhoto(QString keyid);

private slots:
    void startEncode(const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &shred, const bool &symetric);
    void fastEncode(const KURL &filetocrypt, const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &symetric);
    void processEnc(KURL, KgpgInterface*);
    void processEncError(const QString &mssge, KgpgInterface*);
    void processDecOver();
    void processDecError(const QString &mssge);
    void slotShredResult(KIO::Job *job);
    void processEncPopup(const QString &fileName);
    void processPopup2(const QString &fileName);

    void slotAddPhotoFinished(int res, KgpgInterface *interface);

private:
    QWidget *m_panel;
    QString m_extension;
    QStringList m_encryptkeys;
    QStringList m_encryptoptions;

    KURL::List m_urlselecteds;
    KURL m_urlselected;
    KPassivePopup *m_pop;

    bool m_popisactive;
    bool m_shred;
    bool m_symetric;
};

#endif // __KGPGLIBRARY_H__
