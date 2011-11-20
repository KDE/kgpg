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
class KGpgTextInterface;
class KGpgItemModel;

class KgpgLibrary : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    explicit KgpgLibrary(QWidget *parent = 0);

    /**
     * set the extension used for the encrypted file (including the '.')
     */
    void setFileExtension(const QString &extension);

signals:
    void encryptionOver();
    void decryptionOver(KUrl);
    void importOver(KgpgLibrary *, QStringList);
    void systemMessage(QString message);
    void photoAdded();

public slots:
    void slotFileEnc(const KUrl::List &urls, const QStringList &opts, KGpgItemModel *model, const KShortcut &goDefaultKey, const QString &defaultKey = QString());
    void slotFileDec(const KUrl &srcUrl, const KUrl &destUrl);

private slots:
    void processEnc(const KUrl &);
    void processEncError(const QString &mssge);
    void processDecOver(int);
    void processEncPopup(const KUrl &url);

private:
    void startEncode(const QStringList &encryptkeys, const QStringList &encryptoptions, const bool symetric);
    void fastEncode(const KUrl &filetocrypt, const QStringList &encryptkeys, const QStringList &encryptoptions, const bool symetric);
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
