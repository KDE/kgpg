/***************************************************************************
                          popuppublic.cpp  -  description
                             -------------------
    begin                : Sat Jun 29 2002
    copyright            : (C) 2002 by Jean-Baptiste Mardelle
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

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QPainter>
#include <QPixmap>
#include <QLabel>

#include <k3listviewsearchline.h>
#include <kactioncollection.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klineedit.h>
#include <klocale.h>
#include <kaction.h>
#include <khbox.h>
#include <kvbox.h>

#include "kgpginterface.h"
#include "kgpgsettings.h"
#include "images.h"
#include "selectpublickeydialog.h"

using namespace KgpgCore;

class KeyViewItem : public K3ListViewItem
{
public:
    KeyViewItem(K3ListView *parent, const QString &name, const QString &mail, const QString &id, const bool &isDefault, const bool &isTrusted) : K3ListViewItem(parent)
    {
        m_default = isDefault;
        m_istrusted = isTrusted;
        setText(0, name);
        setText(1, mail);
        setText(2, id);
    }

    virtual void paintCell(QPainter *p, const QColorGroup &cg, const int &column, const int &width, const int &alignment)
    {
        if (m_default && column < 2)
        {
            QFont font(p->font());
            font.setBold(true);
            p->setFont(font);
        }

        K3ListViewItem::paintCell(p, cg, column, width, alignment);
    }

    virtual QString key(const int &c) const
    {
        return text(c).toLower();
    }

    virtual bool isTrusted() const
    {
        return m_istrusted;
    }

private:
    bool m_default;
    bool m_istrusted;
};

class KgpgListViewSearchLine : public K3ListViewSearchLine
{
public:
    KgpgListViewSearchLine (QWidget *parent = 0, KgpgSelectPublicKeyDlg *dialog = 0, K3ListView *listView = 0) : K3ListViewSearchLine(parent, listView)
    {
        setDialog(dialog);
    }

    KgpgListViewSearchLine (QWidget *parent, const QList<K3ListView*> &listViews, KgpgSelectPublicKeyDlg *dialog = 0) : K3ListViewSearchLine(parent, listViews)
    {
        setDialog(dialog);
    }

    void setDialog(KgpgSelectPublicKeyDlg *dialog)
    {
        m_dialog = dialog;
    }

protected:
    virtual bool itemMatches (const Q3ListViewItem *listitem, const QString &s) const
    {
        const KeyViewItem *item = dynamic_cast<const KeyViewItem*>(listitem);
	if (!item)
		return false;

        if (m_dialog != 0)
        {
            if (m_dialog->getUntrusted())
                return K3ListViewSearchLine::itemMatches(item, s);

            if (item->isTrusted())
                return K3ListViewSearchLine::itemMatches(item, s);

            return false;
        }

        return K3ListViewSearchLine::itemMatches(item, s);
    }

private:
    KgpgSelectPublicKeyDlg *m_dialog;
};

KgpgSelectPublicKeyDlg::KgpgSelectPublicKeyDlg(QWidget *parent, const QString &sfile, const bool &filemode, const bool &enabledshred, const KShortcut &goDefaultKey)
                      : KDialog(parent)
{
    setCaption(i18n("Select Public Key"));
    setButtons(Details | Ok | Cancel);
    setDefaultButton(Ok);
    setButtonText(Details, i18n("O&ptions"));

    m_fmode = filemode;
    if (m_fmode)
        setCaption(i18n("Select Public Key for %1", sfile));

    QWidget *page = new QWidget(this);

    m_searchbar = new KHBox(page);
    m_searchbar->setSpacing(spacingHint());
    m_searchbar->setFrameShape(QFrame::StyledPanel);


    QLabel *searchlabel = new QLabel(i18n("&Search: "), m_searchbar);

    m_searchlineedit = new KgpgListViewSearchLine(m_searchbar, this);
    searchlabel->setBuddy(m_searchlineedit);

    m_keyslist = new K3ListView(page);
    m_keyslist->setFullWidth(true);
    m_keyslist->setRootIsDecorated(false);
    m_keyslist->setShowSortIndicator(true);
    m_keyslist->setAllColumnsShowFocus(true);
    m_keyslist->addColumn(i18n("Name"));
    m_keyslist->addColumn(i18n("Email"));
    m_keyslist->addColumn(i18n("ID"));
    m_keyslist->setSelectionModeExt(K3ListView::Extended);
    m_keyslist->setWhatsThis(i18n("<b>Public keys list</b>: select the key that will be used for encryption."));
    m_searchlineedit->setListView(m_keyslist);

    KVBox *optionsbox = new KVBox();
    optionsbox->setFrameShape(QFrame::StyledPanel);
    setDetailsWidget(optionsbox);

    m_cbarmor = new QCheckBox(i18n("ASCII armored encryption"), optionsbox);
    m_cbarmor->setChecked(KGpgSettings::asciiArmor());
    m_cbarmor->setWhatsThis(i18n("<b>ASCII encryption</b>: makes it possible to open the encrypted file/message in a text editor"));

    m_cbuntrusted = new QCheckBox(i18n("Allow encryption with untrusted keys"), optionsbox);
    m_cbuntrusted->setChecked(KGpgSettings::allowUntrustedKeys());
    m_cbuntrusted->setWhatsThis(i18n("<b>Allow encryption with untrusted keys</b>: when you import a public key, it is usually "
                    "marked as untrusted and you cannot use it unless you sign it in order to make it 'trusted'. Checking this "
                    "box enables you to use any key, even if it has not be signed."));

    m_cbhideid = new QCheckBox(i18n("Hide user id"), optionsbox);
    m_cbhideid->setChecked(KGpgSettings::hideUserID());
    m_cbhideid->setWhatsThis(i18n("<b>Hide user ID</b>: Do not put the keyid into encrypted packets. This option hides the receiver "
                    "of the message and is a countermeasure against traffic analysis. It may slow down the decryption process because "
                    "all available secret keys are tried."));

    m_cbshred = 0;
    if (filemode)
    {
        KHBox *shredbox = new KHBox(optionsbox);
        shredbox->setMargin(0);

        m_cbshred = new QCheckBox(i18n("Shred source file"), shredbox);
        m_cbshred->setWhatsThis(i18n("<b>Shred source file</b>: permanently remove source file. No recovery will be possible"));
        m_cbshred->setEnabled(enabledshred);
        m_cbshred->setChecked(KGpgSettings::shredSource());

        QString shredWhatsThis = i18n("<qt><b>Shred source file:</b><br /><p>Checking this option will shred (overwrite several times before erasing) the files you have encrypted. This way, it is almost impossible that the source file is recovered.</p><p><b>But you must be aware that this is not secure</b> on all file systems, and that parts of the file may have been saved in a temporary file or in the spooler of your printer if you previously opened it in an editor or tried to print it. Only works on files (not on folders).</p></qt>");

        QLabel *labelshredwt = new QLabel(i18n("<a href=\"whalinehis:%1\">Read this before using shredding</a>", shredWhatsThis), shredbox);
        labelshredwt->setOpenExternalLinks(true);
        labelshredwt->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    }

    m_cbsymmetric = new QCheckBox(i18n("Symmetrical encryption"), optionsbox);
    m_cbsymmetric->setWhatsThis(i18n("<b>Symmetrical encryption</b>: encryption does not use keys. You just need to give a password "
                                     "to encrypt/decrypt the file"));

    QVBoxLayout *dialoglayout = new QVBoxLayout(page);
    dialoglayout->setSpacing(spacingHint());
    dialoglayout->setMargin(0);
    dialoglayout->addWidget(m_searchbar);
    dialoglayout->addWidget(m_keyslist);
    page->setLayout(dialoglayout);

    m_customoptions = 0;
    if (KGpgSettings::allowCustomEncryptionOptions())
    {
        KHBox *expertbox = new KHBox(page);
        (void) new QLabel(i18n("Custom option:"), expertbox);

        m_customoptions = new KLineEdit(expertbox);
        m_customoptions->setText(KGpgSettings::customEncryptionOptions());
        m_customoptions->setWhatsThis(i18n("<b>Custom option</b>: for experienced users only, allows you to enter a gpg command line option, like: '--armor'"));

        dialoglayout->addWidget(expertbox);
    }

    KActionCollection *actcol = new KActionCollection(this);
    KAction *action = new KAction(i18n("&Go to Default Key"), actcol, "go_default_key");
    action->setShortcut(goDefaultKey);

    connect(action, SIGNAL(triggered(bool)), SLOT(slotGotoDefaultKey()));
    connect(m_cbsymmetric, SIGNAL(toggled(bool)), this, SLOT(slotSymmetric(bool)));
    connect(m_cbuntrusted, SIGNAL(toggled(bool)), this, SLOT(slotUntrusted(bool)));
    connect(m_keyslist, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(m_keyslist, SIGNAL(doubleClicked(Q3ListViewItem *, const QPoint &, int)), this, SLOT(slotOk()));

    slotFillKeysList();

    setMinimumSize(550, 200);
    updateGeometry();

    setMainWidget(page);
}

QStringList KgpgSelectPublicKeyDlg::selectedKeys() const
{
    if (getSymmetric())
        return QStringList();

    QStringList selectedKeys;

    QList<Q3ListViewItem*> list = m_keyslist->selectedItems();
    for (int i = 0; i < list.count(); ++i)
        if (list.at(i) && list.at(i)->isVisible())
        {
            if (!list.at(i)->text(2).isEmpty())
                selectedKeys << list.at(i)->text(2); // get the id
            else
                selectedKeys << list.at(i)->text(0); // get the name
        }

    return selectedKeys;
}

bool KgpgSelectPublicKeyDlg::getSymmetric() const
{
    return m_cbsymmetric->isChecked();
}

QString KgpgSelectPublicKeyDlg::getCustomOptions() const
{
    if (m_customoptions == 0)
        return QString();
    return m_customoptions->text().simplified();
}

bool KgpgSelectPublicKeyDlg::getUntrusted() const
{
    return m_cbuntrusted->isChecked();
}

bool KgpgSelectPublicKeyDlg::getArmor() const
{
    return m_cbarmor->isChecked();
}

bool KgpgSelectPublicKeyDlg::getHideId() const
{
    return m_cbhideid->isChecked();
}

bool KgpgSelectPublicKeyDlg::getShred() const
{
    return m_fmode && m_cbshred->isChecked();
}

void KgpgSelectPublicKeyDlg::slotOk()
{
    if (!getSymmetric() && !m_keyslist->selectedItems().isEmpty())
        slotButtonClicked(Ok);
}

void KgpgSelectPublicKeyDlg::slotFillKeysList()
{
    m_keyslist->clear();

    KgpgInterface *interface = new KgpgInterface();
    connect(interface, SIGNAL(readPublicKeysFinished(KgpgCore::KeyList, KgpgInterface*)), this, SLOT(slotFillKeysListReady(KgpgCore::KeyList, KgpgInterface*)));
    interface->readPublicKeys();
}

void KgpgSelectPublicKeyDlg::slotFillKeysListReady(KeyList keys, KgpgInterface *interface)
{
    delete interface;

    /* Add groups */
    QStringList groups = KGpgSettings::groups().split(",");
    if (!groups.isEmpty())
        for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
            if (!QString(*it).isEmpty())
            {
                KeyViewItem *item = new KeyViewItem(m_keyslist, QString(*it), QString::null, QString::null, false, true);
                item->setPixmap(0, Images::group());
            }
    /* */

    /* Get the secret keys list */
    interface = new KgpgInterface();
    KeyList list = interface->readSecretKeys(true);
    delete interface;

    QString m_seclist = QString::null;
    for (int i = 0; i < list.size(); ++i)
        m_seclist += ", 0x" + list.at(i).id();
    /* */

    for (int i = 0; i < keys.size(); ++i)
    {
        bool dead = false;
        Key key = keys.at(i);
        QString id = QString("0x" + key.id());

        KeyTrust c = key.trust();
        bool istrusted = true;
        if (c == TRUST_UNKNOWN || c == TRUST_UNDEFINED || c == TRUST_NONE || c == TRUST_MARGINAL)
            istrusted = false;
        else
        if (c == TRUST_INVALID || c == TRUST_DISABLED || c == TRUST_REVOKED || c == TRUST_EXPIRED)
            dead = true;
        else
        if (c != TRUST_FULL && c != TRUST_ULTIMATE)
            istrusted = false;

        if (key.valide() == false)
            dead = true;

        QString keyname = KgpgInterface::checkForUtf8(key.name());
        if (!dead && !keyname.isEmpty())
        {
            QString defaultKey = KGpgSettings::defaultKey().right(8);
            bool isDefaultKey = false;
            if (key.id() == defaultKey)
                isDefaultKey = true;

            KeyViewItem *item = new KeyViewItem(m_keyslist, keyname, key.email(), id, isDefaultKey, istrusted);

            if (m_seclist.contains(id, Qt::CaseInsensitive))
                item->setPixmap(0, Images::pair());
            else
                item->setPixmap(0, Images::single());
        }
    }

    slotPreSelect();
    slotSelectionChanged();
    slotUntrusted(m_cbuntrusted->checkState() == Qt::Checked);
}

void KgpgSelectPublicKeyDlg::slotPreSelect()
{
    Q3ListViewItem *it = 0;
    if (!m_keyslist->firstChild())
        return;

    if (m_fmode)
        it = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);

    if (!m_cbuntrusted->isChecked())
        slotShowAllKeys();

    if (m_fmode)
    {
        m_keyslist->clearSelection();
        m_keyslist->setSelected(it, true);
        m_keyslist->setCurrentItem(it);
        m_keyslist->ensureItemVisible(it);
    }

    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotSelectionChanged()
{
    if (getSymmetric())
        enableButtonOk(true);
    else
        enableButtonOk(!m_keyslist->selectedItems().isEmpty());
}

void KgpgSelectPublicKeyDlg::slotSymmetric(const bool &state)
{
    m_keyslist->setDisabled(state);
    m_cbuntrusted->setDisabled(state);
    m_cbhideid->setDisabled(state);
    m_searchbar->setDisabled(state);
    slotSelectionChanged();
}

void KgpgSelectPublicKeyDlg::slotUntrusted(const bool &state)
{
    if (state)
    {
        slotShowAllKeys();
        m_searchlineedit->updateSearch();
    }
    else
    {
        m_searchlineedit->updateSearch();
        slotHideUntrustedKeys();
    }
}

void KgpgSelectPublicKeyDlg::slotShowAllKeys()
{
    Q3ListViewItem *current = m_keyslist->firstChild();
    if (current == 0)
        return;

    current->setVisible(true);
    while (current->nextSibling())
    {
        current = current->nextSibling();
        current->setVisible(true);
    }

    m_keyslist->ensureItemVisible(m_keyslist->currentItem());
}

void KgpgSelectPublicKeyDlg::slotHideUntrustedKeys()
{
    KeyViewItem *current = static_cast<KeyViewItem*>(m_keyslist->firstChild());
    if (current == 0)
        return;

    bool reselect = false;

    do
    {
        if (!current->isTrusted())
        {
            if (current->isSelected())
            {
                reselect = true;
                current->setSelected(false);
            }
            current->setVisible(false);
        }

        current = static_cast<KeyViewItem*>(current->nextSibling());

    } while (current);

    bool itemvisible = (m_keyslist->currentItem() != 0) ? m_keyslist->currentItem()->isVisible() : false;
    if (reselect && !itemvisible)
    {
        KeyViewItem *firstvisible = static_cast<KeyViewItem*>(m_keyslist->firstChild());
        while (firstvisible->isVisible() != true)
        {
            firstvisible = static_cast<KeyViewItem*>(firstvisible->nextSibling());
            if (firstvisible == 0)
                return;
        }

        m_keyslist->setSelected(firstvisible, true);
        m_keyslist->setCurrentItem(firstvisible);
        m_keyslist->ensureItemVisible(firstvisible);
    }
}

void KgpgSelectPublicKeyDlg::slotGotoDefaultKey()
{
    Q3ListViewItem *myDefaulKey = m_keyslist->findItem(KGpgSettings::defaultKey(), 2);
    m_keyslist->clearSelection();
    m_keyslist->setCurrentItem(myDefaulKey);
    m_keyslist->setSelected(myDefaulKey, true);
    m_keyslist->ensureItemVisible(myDefaulKey);
}

#include "selectpublickeydialog.moc"
