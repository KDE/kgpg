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

    m_md5sum = checkfile.hexDigest().constData();
    f.close();

    QWidget *page = new QWidget(this);

    QLabel *firstlabel = new QLabel(page);
    firstlabel->setText(i18n("MD5 sum for <b>%1</b> is:", url.fileName()));

    KLineEdit *md5lineedit = new KLineEdit(m_md5sum, page);
    md5lineedit->setReadOnly(true);

    m_led = new KLed(QColor(80, 80, 80), KLed::Off, KLed::Sunken, KLed::Circular, page);
    QSizePolicy policy((QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0);
    policy.setVerticalStretch(0);
    policy.setHorizontalStretch(0);
    policy.setHeightForWidth(m_led->sizePolicy().hasHeightForWidth());
    m_led->setSizePolicy(policy);

    m_label = new QLabel(page);
    m_label->setText(i18n("<b>Unknown status</b>"));

    QHBoxLayout *ledlayout = new QHBoxLayout();
    ledlayout->addWidget(m_led);
    ledlayout->addWidget(m_label);

    QVBoxLayout *dialoglayout = new QVBoxLayout(page);
    dialoglayout->setMargin(marginHint());
    dialoglayout->setSpacing(spacingHint());
    dialoglayout->addWidget(firstlabel);
    dialoglayout->addWidget(md5lineedit);
    dialoglayout->addLayout(ledlayout);
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
        if (text.length() != m_md5sum.length())
            KMessageBox::sorry(this, i18n("Clipboard content is not a MD5 sum."));
        else
        if (text == m_md5sum)
        {
            m_label->setText(i18n("<b>Correct checksum</b>, file is ok."));
            m_led->setColor(QColor(Qt::green));
            m_led->on();
        }
        else
        {
            m_label->setText(i18n("<b>Wrong checksum, FILE CORRUPTED</b>"));
            m_led->setColor(QColor(Qt::red));
            m_led->on();
        }
    }
}

#include "kgpgmd5widget.moc"
