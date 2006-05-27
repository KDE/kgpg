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

    QFile f(url.path());
    f.open(QIODevice::ReadOnly);

    KMD5 checkfile;
    checkfile.reset();
    checkfile.update(f);

    m_mdsum = checkfile.hexDigest().constData();
    f.close();

    QWidget *page = new QWidget(this);

    QLabel *textlabel = new QLabel(page);
    textlabel->setText(i18n("MD5 sum for <b>%1</b> is:", url.fileName()));

    KLineEdit *restrictedline = new KLineEdit(m_mdsum, page);
    restrictedline->setReadOnly(true);

    m_kled = new KLed(QColor(80, 80, 80), KLed::Off, KLed::Sunken, KLed::Circular, page);
    QSizePolicy policy((QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0);
    policy.setVerticalStretch(0);
    policy.setHorizontalStretch(0);
    policy.setHeightForWidth(m_kled->sizePolicy().hasHeightForWidth());
    m_kled->setSizePolicy(policy);

    m_textlabel = new QLabel(page);
    m_textlabel->setText(i18n("<b>Unknown status</b>"));

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_kled);
    layout->addWidget(m_textlabel);

    QVBoxLayout *dialoglayout = new QVBoxLayout(page);
    dialoglayout->setMargin(marginHint());
    dialoglayout->setSpacing(spacingHint());
    dialoglayout->addWidget(textlabel);
    dialoglayout->addWidget(restrictedline);
    dialoglayout->addLayout(layout);
    dialoglayout->addStretch();

    page->show();
    setMainWidget(page);

    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));
}

void Md5Widget::slotApply()
{
    QString text = QApplication::clipboard()->text(QClipboard::Clipboard).remove(' ');
    if (!text.isEmpty())
    {
        if (text.length() != m_mdsum.length())
            KMessageBox::sorry(this, i18n("Clipboard content is not a MD5 sum."));
        else
        if (text == m_mdsum)
        {
            m_textlabel->setText(i18n("<b>Correct checksum</b>, file is ok."));
            m_kled->setColor(QColor(Qt::green));
            m_kled->on();
        }
        else
        {
            m_textlabel->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
            m_kled->setColor(QColor(Qt::red));
            m_kled->on();
        }
    }
}

#include "kgpgmd5widget.moc"
