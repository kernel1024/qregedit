#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

#include "userdialog.h"
#include "ui_userdialog.h"
#include "ui_listdialog.h"

static const QStringList acb_fields = {
    QSL("Disabled"),
    QSL("Home directory required"),
    QSL("Password not required"),
    QSL("Temporary duplicate account"),
    QSL("Normal account"),
    QSL("MNS logon user account"),
    QSL("Interdomain trust account"),
    QSL("Workstation trust account"),
    QSL("Server trust account"),
    QSL("Password don't expire"),
    QSL("Account autolocked"),
    QSL("(unknown 0x08)"),
    QSL("(unknown 0x10)"),
    QSL("(unknown 0x20)"),
    QSL("(unknown 0x40)"),
    QSL("(unknown 0x80)")
};

CUserDialog::CUserDialog(QWidget *parent, int hive_idx, int rid) :
    QDialog(parent),
    ui(new Ui::CUserDialog),
    m_rid(rid),
    m_hive(cgl->reg->getHivePtr(hive_idx))
{
    ui->setupUi(this);

    if (m_hive==nullptr || m_hive->type!=HTYPE_SAM) {
        qWarning() << "Unable to load user data. This is not SAM hive.";
        m_hive = nullptr;
    }

    connect(ui->btnUnlock,&QPushButton::clicked,this,&CUserDialog::unlockAccount);
    connect(ui->btnPromote,&QPushButton::clicked,this,&CUserDialog::promoteUser);
    connect(ui->btnClearPassword,&QPushButton::clicked,this,&CUserDialog::clearPassword);
    connect(ui->btnAddToGroup,&QPushButton::clicked,this,&CUserDialog::addToGroup);
    connect(ui->btnRemoveFromGroup,&QPushButton::clicked,this,&CUserDialog::removeFromGroup);

    for (int i=0;i<acb_fields.count();i++) {
        auto *cb = findChild<QCheckBox *>(QSL("cbACB%1").arg(i+1));
        if (cb==nullptr) continue;
        cb->setText(acb_fields.at(i));
    }

    reloadUserInfo();
}

CUserDialog::~CUserDialog()
{
    if (m_user!=nullptr)
        delete m_user;
    m_user = nullptr;
    delete ui;
}

void CUserDialog::reloadUserInfo()
{
    if (m_hive==nullptr || m_rid<=0) {
        QMessageBox::critical(parentWidget(),tr("QRegEdit error"),
                              tr("Unable to load user data."));
        return;
    }

    const QList<CUser> ul = cgl->reg->listUsers(m_hive);
    const int uidx = ul.indexOf(CUser(m_rid));
    if (uidx<0) {
        QMessageBox::critical(parentWidget(),tr("QRegEdit error"),
                              tr("Unable to find %1 RID for user.").arg(m_rid));
        return;
    }

    if (m_user!=nullptr)
        delete m_user;
    m_user = new CUser(ul.at(uidx));
    ui->editRID->setText(
        QSL("0x%1 (%2)").arg(static_cast<quint16>(m_user->rid), 3, 16, QChar('0')).arg(m_user->rid));
    ui->editRID->setCursorPosition(0);
    ui->editUsername->setText(m_user->username);
    ui->editUsername->setCursorPosition(0);
    ui->editFullname->setText(m_user->fullname);
    ui->editFullname->setCursorPosition(0);
    ui->editComment->setText(m_user->comment);
    ui->editComment->setCursorPosition(0);
    ui->editHomeDir->setText(m_user->homeDir);
    ui->editHomeDir->setCursorPosition(0);

    ui->listGroups->clear();
    const QList<CGroup> grps = cgl->reg->listGroups(m_hive);
    for (const auto gid : qAsConst(m_user->groupIDs)) {
        const int gidx = grps.indexOf(CGroup(gid));
        auto *itm = new QListWidgetItem();
        if (gidx>=0) {
            itm->setText(QSL("(0x%1) %2")
                             .arg(static_cast<quint16>(gid), 3, 16, QChar('0'))
                             .arg(grps.at(gidx).name));
        } else {
            itm->setText(
                QSL("(0x%1) <unknown group>").arg(static_cast<quint16>(gid), 3, 16, QChar('0')));
        }
        itm->setData(Qt::UserRole,gid);
        ui->listGroups->addItem(itm);
    }

    QByteArray fba = cgl->reg->readFValue(m_hive,m_user->rid);
    const int max_sam_lock = sam_get_lockoutinfo(m_hive, 0);
    auto *f = reinterpret_cast<struct user_F *>(fba.data());
    const unsigned short acb = f->ACB_bits;

    ui->groupACB->setTitle(tr("Account bits (0x%1)").arg(acb,4,16,QChar('0')));
    for (int i=0;i<acb_fields.count();i++) {
        auto *cb = findChild<QCheckBox *>(QSL("cbACB%1").arg(i+1));
        if (cb==nullptr) continue;
        cb->setChecked((acb & (1<<i))>0);
    }
    ui->labelLoginCount->setText(tr("Failed login count: %1, while max tries is: %2, "
                                    "total login count: %3.")
                                 .arg(f->failedcnt)
                                 .arg(max_sam_lock)
                                 .arg(f->logins));

    ui->btnClearPassword->setEnabled(!m_user->is_blank_pw);
    ui->btnPromote->setEnabled(!m_user->is_admin);

    if (m_user->is_locked) {
        ui->btnUnlock->setText(QSL("Unlock"));
    } else {
        ui->btnUnlock->setText(QSL("Lock"));
    }
}

void CUserDialog::unlockAccount()
{
    if (m_hive==nullptr || m_user==nullptr) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("User data not loaded."));
        return;
    }
    QByteArray fba = cgl->reg->readFValue(m_hive,m_user->rid);
    auto *f = reinterpret_cast<struct user_F *>(fba.data());

    if (m_user->is_locked) {
        // reset to default sane sets of bits and null failed login counter
        f->ACB_bits |= ACB_PWNOEXP;
        f->ACB_bits &= ~ACB_DISABLED;
        f->ACB_bits &= ~ACB_AUTOLOCK;
    } else {
        f->ACB_bits |= ACB_DISABLED;
    }
    f->failedcnt = 0;

    if (!cgl->reg->writeFValue(m_hive,m_user->rid,fba))
        QMessageBox::critical(this,tr("QRegEdit error"), tr("Failed to update F-value for user."));

    reloadUserInfo();
}

void CUserDialog::promoteUser()
{
    if (m_hive==nullptr || m_user==nullptr) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("User data not loaded."));
        return;
    }

    // Adding to 0x220 (Administrators) ...
    if (sam_add_user_to_grp(m_hive, m_user->rid, 0x220) == 0) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("Adding user to built-in \"Administrators\" group failed.\n"
                                 "Nothing changed."));
        return;
    }
    // Adding to 0x221 (Users) ...
    if (sam_add_user_to_grp(m_hive, m_user->rid, 0x221) == 0) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("Adding user to built-in \"Users\" group failed,\n"
                                 "but user added to built-in \"Administrators\" group."));
        return;
    }

    // Removing from 0x222 (Guests) ...
    if (sam_remove_user_from_grp(m_hive, m_user->rid, 0x222) == 0) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("Removing user from built-in \"Guests\" group failed,\n"
                                 "but user added to built-in \"Administrators\" and \"Users\" groups."));
        return;
    }

    reloadUserInfo();
    QMessageBox::information(this,tr("QRegEdit accounts editor"),
                             tr("Successfully promoted current user to administrator."));
}

void CUserDialog::clearPassword()
{
    if (m_hive==nullptr || m_user==nullptr) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("User data not loaded."));
        return;
    }

    QByteArray vba = cgl->reg->readVValue(m_hive,m_user->rid);
    auto *v = reinterpret_cast<struct user_V *>(vba.data());

    /* Setting hash lengths to zero seems to make NT think it is blank.
     * However, since we cant cut the previous hash bytes out of the V value
     * due to missing resize-support of values, it may leak about 40 bytes
     * each time we do this.
     */
    v->ntpw_len = 0;
    v->lmpw_len = 0;

    if (!cgl->reg->writeVValue(m_hive,m_user->rid,vba))
        QMessageBox::critical(this,tr("QRegEdit error"), tr("Failed to update V-value for user."));

    reloadUserInfo();
}

void CUserDialog::addToGroup()
{
    if (m_hive==nullptr || m_user==nullptr) {
        QMessageBox::critical(this,tr("QRegEdit error"), tr("User data not loaded."));
        return;
    }

    auto *dlg = new QDialog(this);
    Ui::CListDialog ldui;
    ldui.setupUi(dlg);

    const QList<CGroup> grps = cgl->reg->listGroups(m_hive);
    for (int i=0;i<grps.count();i++) {
        const int gid = grps.at(i).grpid;
        if (!m_user->groupIDs.contains(gid))
            ldui.list->addItem(grps.at(i).name,gid);
    }

    if (dlg->exec()==QDialog::Accepted) {
        bool ok = false;
        const int grpid = ldui.list->currentData().toInt(&ok);

        if (sam_add_user_to_grp(m_hive, m_rid, grpid) == 0)
            QMessageBox::critical(this,tr("QRegEdit error"), tr("Failed to add user to group."));

        reloadUserInfo();
    }
    dlg->setParent(nullptr);
    dlg->deleteLater();
}

void CUserDialog::removeFromGroup()
{
    if (m_hive==nullptr || m_user==nullptr) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("User data not loaded."));
        return;
    }

    QListWidgetItem *itm = ui->listGroups->currentItem();
    if (itm==nullptr || itm->data(Qt::UserRole).isNull() ||
            !itm->data(Qt::UserRole).canConvert<int>()) {
        QMessageBox::warning(this,tr("QRegEdit user editor"),
                             tr("Please select group from list in user editor dialog."));
        return;
    }

    const int grp = itm->data(Qt::UserRole).toInt();

    if (sam_remove_user_from_grp(m_hive, m_rid, grp) == 0) {
        QMessageBox::critical(this,tr("QRegEdit error"),
                              tr("Failed to remove user from group."));
    }

    reloadUserInfo();
}
