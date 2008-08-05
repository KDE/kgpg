/**************************************************************************
                          kgpgview.h  -  description
                             -------------------
    begin                : Tue Jul  2 12:31:38 GMT 2002
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

#ifndef KGPGVIEW_H
#define KGPGVIEW_H

#include <KTextEdit>
#include <KUrl>

class QDragEnterEvent;
class QDropEvent;

class KGpgTextInterface;
class KGpgItemModel;

class KgpgTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    explicit KgpgTextEdit(QWidget *parent, KGpgItemModel *model);
    ~KgpgTextEdit();

signals:
    void newText();
    void resetEncoding(bool);
    void verifyFinished();

public slots:
    void slotDroppedFile(const KUrl &url);
    void slotEncode();
    void slotDecode();
    void slotSign();
    void slotVerify();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

private:
    void deleteFile();
    bool checkForUtf8(const QString &text);

private slots:
    void slotDecodeFile();
    bool slotCheckFile(const bool &checkforpgpmessage = true);

    void slotDecodeFileSuccess(const QByteArray &content, KGpgTextInterface *interface);
    void slotDecodeFileFailed(const QString &content, KGpgTextInterface *interface);

    void slotEncodeUpdate(const QString &content, KGpgTextInterface *interface);
    void slotDecodeUpdateSuccess(const QByteArray &content, KGpgTextInterface *interface);
    void slotDecodeUpdateFailed(const QString &content, KGpgTextInterface *interface);
    void slotSignUpdate(const QString &content, KGpgTextInterface *interface);
    void slotVerifySuccess(const QString &content, const QString &log, KGpgTextInterface *interface);
    void slotVerifyKeyNeeded(const QString &id, KGpgTextInterface *interface);

private:
    QString m_tempfile;

    int m_posstart;
    int m_posend;

    KGpgItemModel *m_model;
};

class KgpgView : public QWidget
{
    Q_OBJECT

public:
    explicit KgpgView(QWidget *parent, KGpgItemModel *model);
    ~KgpgView();

    KgpgTextEdit *editor;

signals:
    void newText();
    void textChanged();
    void resetEncoding(bool);
    void verifyFinished();

public slots:
    void slotSignVerify();
    void slotEncode();
    void slotDecode();
    void slotHighlightText(const QString &, const int &matchingindex, const int &matchedlength);
};

#endif // KGPGVIEW_H
