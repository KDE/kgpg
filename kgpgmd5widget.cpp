/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>
#include <QHBoxLayout>
#include <QClipboard>
#include <QLabel>
#include <QFile>

#include <kmessagebox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kdebug.h>
#include <kled.h>

#include "kgpgmd5widget.h"

Md5Widget::Md5Widget(QWidget *parent, const KUrl &url)
         : KDialog(parent, i18n("MD5 Checksum"), Apply | Close)
{
    setModal(true);
    setButtonGuiItem(Apply, i18n("Compare MD5 with Clipboard"));

    m_mdsum = QString::null;

    QFile f(url.path());
    f.open(QIODevice::ReadOnly);

    KMD5 checkfile;
    checkfile.reset();
    checkfile.update(f);

    m_mdsum = checkfile.hexDigest().constData();
    f.close();

    QWidget *page = new QWidget(this);

    resize(360, 150);
    QGridLayout *dialoglayout = new QGridLayout(page );
    dialoglayout->setMargin( 5 );
    dialoglayout->setSpacing( 6 );
    dialoglayout->setObjectName( "MyDialogLayout" );

    QLabel *textlabel = new QLabel(page, "TextLabel1");
    textlabel->setText(i18n("MD5 sum for <b>%1</b> is:", url.fileName()));
    dialoglayout->addWidget(textlabel, 0, 0);

    KLineEdit *restrictedline = new KLineEdit(m_mdsum, page);
    restrictedline->setReadOnly(true);
    dialoglayout->addWidget(restrictedline, 1, 0);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setObjectName("Layout4");
    layout->setSpacing(6);
    layout->setMargin(0);
    m_kled = new KLed(QColor(80, 80, 80), KLed::Off, KLed::Sunken, KLed::Circular, page);
    m_kled->off();
    m_kled->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, m_kled->sizePolicy().hasHeightForWidth()));
    layout->addWidget(m_kled);

    m_textlabel = new QLabel(page, "m_textlabel");
    m_textlabel->setText(i18n("<b>Unknown status</b>"));
    layout->addWidget(m_textlabel);

    dialoglayout->addLayout(layout, 2, 0);

    QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    dialoglayout->addItem(spacer, 3, 0);

    page->show();
    page->resize(page->minimumSize());

    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

    setMainWidget(page);
}

void Md5Widget::slotApply()
{
    QClipboard *cb = QApplication::clipboard();
    QString text;

    text = cb->text(QClipboard::Clipboard).remove(' ');
    if (!text.isEmpty())
    {
        if (text == m_mdsum)
        {
            m_textlabel->setText(i18n("<b>Correct checksum</b>, file is ok."));
            m_kled->setColor(QColor(0, 255, 0));
            m_kled->on();
        }
        else
        if (text.length() != m_mdsum.length())
            KMessageBox::sorry(0, i18n("Clipboard content is not a MD5 sum."));
        else
        {
            m_textlabel->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
            m_kled->setColor(QColor(255, 0, 0));
            m_kled->on();
        }
    }
}

#include "kgpgmd5widget.moc"
