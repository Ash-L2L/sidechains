// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/switchbitnamesdialog.h>
#include <qt/forms/ui_switchbitnamesdialog.h>

#include <qt/claimbitnamedialog.h>
#include <qt/registerbitnamedialog.h>
#include <qt/recoverbitnamedialog.h>

#include <QMessageBox>

#include <base58.h>
#include <primitives/transaction.h>
#include <txdb.h>
#include <validation.h>
#include <wallet/wallet.h>

SwitchBitNamesDialog::SwitchBitNamesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SwitchBitNamesDialog)
{
    ui->setupUi(this);


    // Table style

#if QT_VERSION < 0x050000
    ui->tableWidgetNames->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetNames->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
    ui->tableWidgetNames->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetNames->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif

    // Hide vertical header
    ui->tableWidgetNames->verticalHeader()->setVisible(false);
    // Left align the horizontal header text
    ui->tableWidgetNames->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    // Set horizontal scroll speed to per 3 pixels
    ui->tableWidgetNames->horizontalHeader()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    // Select entire row
    ui->tableWidgetNames->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Select only one row
    ui->tableWidgetNames->setSelectionMode(QAbstractItemView::SingleSelection);
    // Disable word wrap
    ui->tableWidgetNames->setWordWrap(false);

    Update();
}

SwitchBitNamesDialog::~SwitchBitNamesDialog()
{
    delete ui;
}

void SwitchBitNamesDialog::on_pushButtonRegister_clicked()
{
    RegisterBitNameDialog dialog;
    dialog.exec();

    Update();
}

void SwitchBitNamesDialog::on_pushButtonClaim_clicked()
{
    ClaimBitNameDialog dialog;
    dialog.exec();
}

void SwitchBitNamesDialog::on_pushButtonRecover_clicked()
{
    RecoverBitNameDialog dialog;
    dialog.exec();
}

void SwitchBitNamesDialog::Update()
{
    // List my bitnames on table

    ui->tableWidgetNames->setRowCount(0);

    if (vpwallets.empty()) {
        QMessageBox::critical(this, tr("Failed to check BitNames!"),
            tr("No active wallet!\n"),
            QMessageBox::Ok);
    }

    EnsureWalletIsUnlocked(vpwallets[0]);
    vpwallets[0]->BlockUntilSyncedToCurrentChain();

    LOCK2(cs_main, vpwallets[0]->cs_wallet);

    std::vector<COutput> vOutput;
    vpwallets[0]->AvailableBitNames(vOutput);

    int nRow = 0;
    for (const COutput& o : vOutput) {
        ui->tableWidgetNames->insertRow(nRow);

        // Get BitNameDB data
        BitName bitname;
        if (!pbitnametree->GetBitName(o.tx->nAssetID, bitname)) {
            return;
        }

        QString name = QString::fromStdString(bitname.strName);
        QString id = QString::fromStdString(bitname.nID.ToString());

        // Add to table
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QTableWidgetItem* idItem = new QTableWidgetItem(id);
        idItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ui->tableWidgetNames->setItem(nRow /* row */, 0 /* col */, nameItem);
        ui->tableWidgetNames->setItem(nRow /* row */, 1 /* col */, idItem);

        nRow++;
    }
}
