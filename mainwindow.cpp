#include <QScreen>
#include <QWindow>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QShortcut>

#include "global.h"
#include "regutils.h"
#include "mainwindow.h"
#include "valueeditor.h"
#include "logdisplay.h"
#include "userdialog.h"
#include "ui_mainwindow.h"

CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    if (cgl == nullptr)
        cgl = new CGlobal(this);

    ui->setupUi(this);

    setWindowIcon(QIcon(QSL(":/icons/qregedit")));

    treeModel = new CRegistryModel(this);
    valuesModel = new CValuesModel(this);
    groupsModel = new CSAMGroupsModel(this);
    usersModel = new CSAMUsersModel(this);
    valuesSortModel = new QSortFilterProxyModel(this);

    valuesSortModel->setSourceModel(valuesModel);
    ui->treeHives->setModel(treeModel);
    ui->tableValues->setModel(valuesSortModel);
    ui->treeGroups->setModel(groupsModel);
    ui->tableUsers->setModel(usersModel);

    ui->treeHives->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableValues->setContextMenuPolicy(Qt::CustomContextMenu);

    centerWindow();

    searchProgressDialog = new CProgressDialog(this);

    ui->actionOpenHiveRO->setData(1);
    connect(ui->actionExit, &QAction::triggered, this, &CMainWindow::close);
    connect(ui->actionOpenHive, &QAction::triggered, this, &CMainWindow::openHive);
    connect(ui->actionOpenHiveRO, &QAction::triggered, this, &CMainWindow::openHive);
    connect(ui->actionImport, &QAction::triggered, this, &CMainWindow::importReg);
    connect(ui->actionAbout, &QAction::triggered, this, &CMainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionSettings, &QAction::triggered, [this]() {
        cgl->settingsDialog(this);
    });

    if (cgl->logWindow != nullptr)
        connect(ui->actionLog, &QAction::triggered, cgl->logWindow.data(), &CLogDisplay::show);

    connect(ui->actionFind, &QAction::triggered, this, &CMainWindow::searchTxt);
    connect(ui->actionFindAgain, &QAction::triggered,
            treeModel->finder.data(), &CFinder::continueSearch, Qt::QueuedConnection);
    connect(treeModel->finder.data(), &CFinder::showProgressDialog,
            searchProgressDialog, &CProgressDialog::show, Qt::QueuedConnection);
    connect(treeModel->finder.data(), &CFinder::hideProgressDialog,
            searchProgressDialog, &CProgressDialog::hide, Qt::QueuedConnection);
    connect(this, &CMainWindow::startSearch,
            treeModel->finder.data(), &CFinder::searchText, Qt::QueuedConnection);
    connect(searchProgressDialog, &CProgressDialog::cancel, [this]() {
        treeModel->finder->cancelSearch();
    });

    connect(ui->treeHives, &QTreeView::clicked, this, &CMainWindow::showValues);
    connect(ui->treeHives, &QTreeView::activated, this, &CMainWindow::showValues);
    connect(ui->treeHives, &QTreeView::customContextMenuRequested,
            this, &CMainWindow::treeCtxMenu);

    connect(ui->tableValues, &QTableView::customContextMenuRequested,
            this, &CMainWindow::valuesCtxMenu);
    connect(ui->tableValues, &QTableView::activated,
            this, &CMainWindow::valuesModify);

    connect(valuesModel,&CValuesModel::valuesReloaded,this,[this](){
        ui->tableValues->resizeColumnsToContents();
        ui->tableValues->sortByColumn(0, Qt::AscendingOrder);
    });
    connect(usersModel,&CSAMUsersModel::valuesReloaded,this,[this](){
        ui->tableUsers->resizeColumnsToContents();
    });

    connect(ui->tableUsers, &QTableView::activated,
            this, &CMainWindow::editUser);

    connect(treeModel, &CRegistryModel::keyFound, this, &CMainWindow::keyFound);
    connect(treeModel->finder.data(), &CFinder::searchFinished,
            this, &CMainWindow::searchFinished, Qt::QueuedConnection);

    connect(cgl->reg.data(), &CRegController::hiveAboutToClose, this, &CMainWindow::hivePrepareClose);

    auto *sc = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableValues);
    connect(sc, &QShortcut::activated, [this]() {
        deleteValue(ui->tableValues->currentIndex());
    });

    ui->tabSAM->hide();

    QStringList errors;
    const QStringList args = QApplication::arguments();

    for (int i = 1; i < args.count(); i++) {
        if (!cgl->reg->openTopHive(args.at(i), HMODE_RW))
            errors.append(args.at(i));
    }

    if (!errors.isEmpty()) {
        QTimer::singleShot(2500, [errors]() {
            QString msg = tr("Failed to open hives:\n");
            msg.append(errors.join('\n'));
            QMessageBox::critical(QApplication::activeWindow(), tr("Registry Editor - Error"), msg);
        });
    }
}

CMainWindow::~CMainWindow()
{
    delete ui;
}

void CMainWindow::centerWindow()
{
    const int initialHeightFrac = 70;
    const int initialWidthFrac = 135;
    const int maxWidth = 1000;
    const int maxWidthFrac = 80;

    QScreen *screen = nullptr;

    if (window() && window()->windowHandle()) {
        screen = window()->windowHandle()->screen();
    } else if (!QApplication::screens().isEmpty()) {
        screen = QApplication::screenAt(QCursor::pos());
    }
    if (screen == nullptr)
        screen = QApplication::primaryScreen();

    const QRect rect(screen->availableGeometry());
    const int h = initialHeightFrac*rect.height()/100;
    QSize nw(initialWidthFrac*h/100,h);
    if (nw.width()<maxWidth) nw.setWidth(maxWidthFrac*rect.width()/100);
    resize(nw);
    move(rect.width()/2 - frameGeometry().width()/2,
         rect.height()/2 - frameGeometry().height()/2);
}

void CMainWindow::closeEvent(QCloseEvent *event)
{
    if (cgl != nullptr && cgl->safeToClose()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void CMainWindow::openHive()
{
    auto *ac = qobject_cast<QAction *>(sender());
    bool b = false;
    const int t = ac->data().toInt(&b);
    int mode = HMODE_RW;

    if (b && (t == 1))
        mode = HMODE_RO;

    const QString fname = getOpenFileNameD(this, tr("Open registry hive"));

    if (!fname.isEmpty()) {
        if (!cgl->reg->openTopHive(fname, mode)) {
            QMessageBox::critical(this, tr("Registry Editor - Error"), tr("Failed to open hive file.\n"
                                  "See log messages for debug messages."));
        }
    }
}

void CMainWindow::importReg()
{
    const int idx = treeModel->getHiveIdx(ui->treeHives->currentIndex());

    if (idx < 0) return;

    const QString fname = getOpenFileNameD(this, tr("Import REG file to selected hive"));

    if (!fname.isEmpty()) {
        if (!cgl->reg->importReg(cgl->reg->getHivePtr(idx), fname)) {
            QMessageBox::critical(this, tr("Registry Editor - Error"),
                                  tr("Failed to import file. See log for error messages.\n"
                                     "Please, do not save hive %1!")
                                  .arg(cgl->reg->getHivePrefix(cgl->reg->getHivePtr(idx))));
        } else {
            QMessageBox::information(this, tr("Registry Editor - Import"),
                                     tr("Registry file import successfull.\n"
                                        "Hive %1 modified.")
                                     .arg(cgl->reg->getHivePrefix(cgl->reg->getHivePtr(idx))));
        }

        ui->treeHives->collapseAll();
    }
}

void CMainWindow::showValues(const QModelIndex &key)
{
    valuesModel->keyChanged(key);

    const int idx = treeModel->getHiveIdx(ui->treeHives->currentIndex());
    bool vis = false;

    if (idx >= 0) {
        struct hive *h = cgl->reg->getHivePtr(idx);
        vis = (h->type == HTYPE_SAM);
    }

    if (vis) {
        ui->tabSAM->show();
        groupsModel->keyChanged(key);
        usersModel->keyChanged(key);
    } else
        ui->tabSAM->hide();
}

void CMainWindow::hivePrepareClose(int idx)
{
    Q_UNUSED(idx)

    valuesModel->keyChanged(QModelIndex());
    groupsModel->keyChanged(QModelIndex());
    usersModel->keyChanged(QModelIndex());
    ui->tabSAM->hide();
}

void CMainWindow::treeCtxMenuPrivate(const QPoint &pos, bool fromValuesTable)
{
    const QModelIndex idx = ui->treeHives->currentIndex();

    const int hive = treeModel->getHiveIdx(idx);
    if (hive < 0) return;

    QMenu *cm = nullptr;

    if (fromValuesTable) {
        cm = new QMenu(ui->tableValues);
    } else {
        if (valuesModel->getCurrentKeyName() != treeModel->getKeyName(idx)) {
            showValues(idx);
            QApplication::processEvents();
        }

        cm = new QMenu(ui->treeHives);
    }

    QAction *acm = nullptr;
    QMenu *ccm = cm->addMenu(tr("New"));
    {
        acm = ccm->addAction(tr("Key"));
        acm->setData(-1);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
        ccm->addSeparator();
        acm = ccm->addAction(tr("String value"));
        acm->setData(REG_SZ);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
        acm = ccm->addAction(tr("Binary value"));
        acm->setData(REG_BINARY);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
        acm = ccm->addAction(tr("DWORD value"));
        acm->setData(REG_DWORD);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
        acm = ccm->addAction(tr("Multistring value"));
        acm->setData(REG_MULTI_SZ);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
        acm = ccm->addAction(tr("Expandable string value"));
        acm->setData(REG_EXPAND_SZ);
        connect(acm, &QAction::triggered, this, &CMainWindow::createEntry);
    }

    if (idx.isValid()) {
        cm->addSeparator();

        acm = cm->addAction(tr("Delete"));

        if (idx.parent().isValid()) {
            connect(acm, &QAction::triggered, [this, idx]() {
                treeModel->deleteKey(idx);
                valuesModel->keyChanged(QModelIndex());
            });
        } else
            acm->setEnabled(false);

        cm->addSeparator();
        acm = cm->addAction(tr("Copy key name"));
        connect(acm, &QAction::triggered, [this, idx]() {
            QApplication::clipboard()->setText(treeModel->getKeyName(idx));
        });
        acm = cm->addAction(tr("Export..."));
        acm->setEnabled(cgl->reg->getHivePtr(hive)->type != HTYPE_SAM);
        connect(acm, &QAction::triggered, [this, idx]() {
            const QString fname = getSaveFileNameD(this, tr("Export registry"), QString(),
                                             tr("Registry files (*.reg)"));

            if (!fname.isEmpty()) {
                if (!treeModel->exportKey(idx, fname)) {
                    QMessageBox::critical(this, tr("Registry Editor - Error"), tr("Failed to export selected key."));
                } else {
                    QMessageBox::information(this, tr("Registry Editor - Export"),
                                             tr("Registry key exported successfully."));
                }
            }
        });

        cm->addSeparator();
        acm = cm->addAction(tr("Find..."));
        connect(acm, &QAction::triggered, [this, idx]() {
            bool ok = false;
            const QString s = QInputDialog::getText(this, tr("Registry Editor - Search"),
                                              tr("Search text"), QLineEdit::Normal, QString(), &ok);

            if (ok && !s.isEmpty())
                Q_EMIT startSearch(idx, s);
        });

        struct hive *h = cgl->reg->getHivePtr(hive);

        if (h != nullptr && h->type == HTYPE_SOFTWARE) {
            cm->addSeparator();
            acm = cm->addAction(tr("Show OS info..."));
            connect(acm, &QAction::triggered, [this, h]() {
                const QString info = cgl->reg->getOSInfo(h);

                if (!info.isEmpty())
                    QMessageBox::information(this, tr("Registry Editor - OS info"), info);
            });
        }
    }

    cm->addSeparator();
    acm = cm->addAction(tr("Close hive"));
    connect(acm, &QAction::triggered, [hive]() {
        cgl->safeToClose(hive);
    });

    if (fromValuesTable) {
        cm->exec(ui->tableValues->mapToGlobal(pos));
    } else {
        cm->exec(ui->treeHives->mapToGlobal(pos));
    }

    cm->deleteLater();
}

void CMainWindow::valuesCtxMenu(const QPoint &pos)
{
    const QModelIndex uidx = ui->tableValues->indexAt(pos);
    const QModelIndex idx = valuesSortModel->mapToSource(uidx);

    const QString name = valuesModel->getValueName(idx);

    QMenu cm(ui->tableValues);
    QAction *acm = nullptr;

    if (idx.isValid()) { // Context menu for value
        acm = cm.addAction(tr("Modify"));
        connect(acm, &QAction::triggered, [this, uidx]() {
            valuesModify(uidx);
        });

        cm.addSeparator();

        acm = cm.addAction(tr("Delete"));
        acm->setDisabled(valuesModel->getValue(idx).isDefault());
        connect(acm, &QAction::triggered, [this, idx]() {
            deleteValue(idx);
        });

        acm = cm.addAction(tr("Rename"));
        connect(acm, &QAction::triggered, [this, name, idx]() {
            QString s = name;
            bool ok = false;
            s = QInputDialog::getText(this, tr("Registry Editor - Value rename"),
                                      tr("Rename registry value"), QLineEdit::Normal, s, &ok);

            if (ok && !s.isEmpty())
                valuesModel->renameValue(idx, s);
        });

        cm.exec(ui->tableValues->mapToGlobal(pos));
    } else {
        treeCtxMenuPrivate(pos, true);
    }
}

void CMainWindow::valuesModify(const QModelIndex &key)
{
    const QModelIndex idx = valuesSortModel->mapToSource(key);

    auto *dlg = new CValueEditor(this, REG_NONE, idx);

    if (!dlg->initFailed())
        dlg->exec();

    dlg->deleteLater();
}

void CMainWindow::createEntry()
{
    auto *ac = qobject_cast<QAction *>(sender());

    if (ac == nullptr) return;

    bool ok = false;
    const int type = ac->data().toInt(&ok);

    if (!ok) return;

    const QModelIndex idx = ui->treeHives->currentIndex();

    if (type == -1 && idx.isValid()) {
        const QString name = QInputDialog::getText(this, tr("Registry Editor - New key"), tr("New key name"));

        if (!name.isEmpty()) {
            if (!treeModel->createKey(idx, name))
                QMessageBox::critical(this, tr("Registry Editor - Error"), tr("Failed to create new key."));
        }

    } else if (type > REG_NONE && type < REG_MAX) {
        auto *dlg = new CValueEditor(this, type, QModelIndex());

        if (!dlg->initFailed()) {
            if (dlg->exec() == QDialog::Accepted) {
                if (!valuesModel->createValue(dlg->getValue()))
                    QMessageBox::critical(this, tr("Registry Editor - Error"), tr("Failed to create new value."));
            }
        }

        dlg->deleteLater();
    }
}

void CMainWindow::about()
{
    QMessageBox::about(this, tr("About"),
                       tr("Windows registry editor, written with Qt.\n" \
                          "(c) kernel1024, 2016 - 2023, GPL v2\n\n" \
                          "Code parts:\n" \
                          "chntpw %1, LGPL v2.1\n" \
                          "QHexEdit widget (c) Winfried Simon, LGPL v2.1")
                       .arg(ntreg_version));
}

void CMainWindow::keyFound(const QModelIndex &key, const QString &value)
{
    ui->treeHives->setCurrentIndex(key);
    showValues(key);

    if (!value.isEmpty()) {
        const QModelIndex idx = valuesSortModel->mapFromSource(valuesModel->getValueIdx(value));

        if (idx.isValid())
            ui->tableValues->setCurrentIndex(idx);
    }
}

void CMainWindow::searchTxt()
{
    const QModelIndex idx = ui->treeHives->currentIndex();

    if (!idx.isValid()) return;

    bool ok = false;
    const QString s = QInputDialog::getText(this, tr("Registry Editor - Search"),
                                            tr("Search text"), QLineEdit::Normal, QString(), &ok);

    if (ok && !s.isEmpty())
        Q_EMIT startSearch(idx, s);
}

void CMainWindow::searchFinished()
{
    QMessageBox::information(this, tr("Registry Editor - Search"), tr("Search completed."));
}

void CMainWindow::deleteValue(const QModelIndex &value)
{
    if (!value.isValid()) return;

    const QString name = valuesModel->getValueName(value);

    if (!valuesModel->deleteValue(value)) {
        QMessageBox::critical(this, tr("Registry Editor - Error"), tr("Failed to delete value '%1'.")
                              .arg(name));
    }
}

void CMainWindow::editUser(const QModelIndex &index)
{
    if (!index.isValid()) return;

    const int rid = usersModel->getUserRID(index);

    if (rid < 0) return;

    auto *dlg = new CUserDialog(this, usersModel->getHiveIdx(), rid);
    dlg->exec();

    dlg->setParent(nullptr);
    delete dlg;
}
