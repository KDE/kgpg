/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
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
class KeysManager;

class KgpgTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    explicit KgpgTextEdit(QWidget *parent, KGpgItemModel *model, KeysManager *manager);
    ~KgpgTextEdit();

    void signVerifyText(const QString &message);
    void openDroppedFile(const KUrl &url, const bool probe);

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
    void slotHighlightText(const QString &, const int matchingindex, const int matchedlength);
    void slotVerifyDone(int result);

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private:
    void verifyKeyNeeded(const QString &id);

private slots:
    void slotEncodeUpdate(int result);
    void slotSignUpdate(int result);

    void slotDecryptDone(int result);

private:
    QString m_tempfile;

    int m_posstart;
    int m_posend;

    KGpgItemModel *m_model;
    KeysManager *m_keysmanager;
};

#endif // KGPGVIEW_H
