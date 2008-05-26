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

#ifndef KGPGLIBRARY_H
#define KGPGLIBRARY_H

#include <QObject>

#include <KShortcut>
#include <KUrl>

#include <QStringList>
#include <QString>

class KPassivePopup;
class KgpgInterface;
class KGpgTextInterface;

class KgpgLibrary : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    explicit KgpgLibrary(QWidget *parent = 0, const bool &pgpExtension = false);

signals:
    void encryptionOver();
    void decryptionOver();
    void importOver(QStringList);
    void systemMessage(QString, bool reset = false);
    void photoAdded();

public slots:
    void slotFileEnc(const KUrl::List &urls = KUrl(""), const QStringList &opts = QStringList(), const QStringList &defaultKey = QStringList(), const KShortcut &goDefaultKey = KShortcut(QKeySequence(Qt::CTRL + Qt::Key_Home)));
    void slotFileDec(const KUrl &srcUrl, const KUrl &destUrl, const QStringList &customDecryptOption = QStringList());

    void addPhoto(const QString &keyid);

private slots:
    void startEncode(const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &symetric);
    void fastEncode(const KUrl &filetocrypt, const QStringList &encryptkeys, const QStringList &encryptoptions, const bool &symetric);
    void processEnc(const KUrl &, KGpgTextInterface*);
    void processEncError(const QString &mssge, KGpgTextInterface*);
    void processDecOver(int, KGpgTextInterface*);
    void processDecError(const QString &mssge);
    void processEncPopup(const KUrl &url);
    void processPopup2(const QString &fileName);

    void slotAddPhotoFinished(int res, KgpgInterface *interface);

private:
    QWidget *m_panel;
    QString m_extension;
    QStringList m_encryptkeys;
    QStringList m_encryptoptions;

    KUrl::List m_urlselecteds;
    KUrl m_urlselected;
    KPassivePopup *m_pop;

    bool m_popisactive;
    bool m_symetric;
};

#endif // KGPGLIBRARY_H
