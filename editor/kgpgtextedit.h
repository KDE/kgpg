/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2009 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGTEXTEDIT_H
#define KGPGTEXTEDIT_H

#include <QString>

#include <KTextEdit>
#include <KUrl>

class QDragEnterEvent;
class QDropEvent;

class KGpgItemModel;

class KgpgTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    explicit KgpgTextEdit(QWidget *parent, KGpgItemModel *model);
    ~KgpgTextEdit();

    /**
     * @brief check if a given text is a GPG key
     * @returns if this is a key and which type of key
     * @retval 0 no key found
     * @retval 1 public key found
     * @retval 2 private key found
     */
    static int checkForKey(const QString &message);

    void signVerifyText(const QString &message);

signals:
    void newText();
    void resetEncoding(bool);
    void verifyFinished();

public slots:
    void slotDroppedFile(const KUrl &url);
    void slotEncode();
    void slotDecode();
    void slotSign(const QString &message);
    void slotVerify(const QString &message);
    void slotSignVerify();
    void slotHighlightText(const QString &, const int &matchingindex, const int &matchedlength);

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private:
    bool checkForUtf8(const QString &text);

private slots:
    void slotEncodeUpdate(const QString &content);
    void slotSignUpdate(const QString &content);
    void slotVerifySuccess(const QString &content, const QString &log);
    void slotVerifyKeyNeeded(const QString &id);

    void slotDecryptDone(int result);

private:
    QString m_tempfile;

    int m_posstart;
    int m_posend;

    KGpgItemModel *m_model;
};

#endif // KGPGVIEW_H
