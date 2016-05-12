#include <QMessageBox>

#include "userdialog.h"
#include "global.h"
#include "ui_userdialog.h"

static const QStringList acb_fields = {
    "Disabled",
    "Homedir required" ,
    "Password not required" ,
    "Temporary duplicate" ,
    "Normal account" ,
    "NMS account" ,
    "Domain trust account" ,
    "Workstation trust account" ,
    "Server trust account" ,
    "Password don't expires" ,
    "Auto lockout" ,
    "(unknown 0x08)" ,
    "(unknown 0x10)" ,
    "(unknown 0x20)" ,
    "(unknown 0x40)" ,
    "(unknown 0x80)"
};

CUserDialog::CUserDialog(QWidget *parent, int hive_idx, int rid) :
    QDialog(parent),
    ui(new Ui::CUserDialog)
{
    ui->setupUi(this);
    m_rid = rid;
    m_hive = hive_idx;

    for (int i=0;i<acb_fields.count();i++) {
        QCheckBox *cb = findChild<QCheckBox *>(QString("cbACB%1").arg(i+1));
        if (cb==NULL) continue;
        cb->setText(acb_fields.at(i));
    }

    reloadUserInfo();
}

CUserDialog::~CUserDialog()
{
    delete ui;
}

void CUserDialog::reloadUserInfo()
{
    if (m_hive<0 || m_rid<=0) {
        QMessageBox::critical(parentWidget(),tr("QRegEdit error"),tr("Unable to load user data."));
        return;
    }
    struct hive *h = cgl->reg->getHivePtr(m_hive);

    QList<CUser> ul = cgl->reg->listUsers(h);
    int uidx = ul.indexOf(CUser(m_rid));
    if (uidx<0) {
        QMessageBox::critical(parentWidget(),tr("QRegEdit error"),tr("Unable to find %1 RID for user.").arg(m_rid));
        return;
    }

    CUser u = ul.at(uidx);
    ui->editRID->setText(QString("0x%1 (%2)").arg((quint16)u.rid,3,16,QChar('0')).arg(u.rid));
    ui->editUsername->setText(u.username);
    ui->editFullname->setText(u.fullname);
    ui->editComment->setText(u.comment);
    ui->editHomeDir->setText(u.homeDir);

    QList<CGroup> grps = cgl->reg->listGroups(h);
    foreach (const int gid, u.groupIDs) {
        int gidx = grps.indexOf(CGroup(gid));
        QListWidgetItem *itm = new QListWidgetItem();
        if (gidx>=0)
            itm->setText(QString("(0x%1) %2").arg((quint16)gid,3,16,QChar('0')).arg(grps.at(gidx).name));
        else
            itm->setText(QString("(0x%1) <unknown group>").arg((quint16)gid,3,16,QChar('0')));
        itm->setIcon(QIcon::fromTheme("user-group-properties"));
        itm->setData(Qt::UserRole,gid);
        ui->listGroups->addItem(itm);
    }

    // TODO: account bits and login counters
}
