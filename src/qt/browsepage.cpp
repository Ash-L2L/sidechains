// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/browsepage.h>
#include <qt/forms/ui_browsepage.h>

#include <qt/platformstyle.h>

#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <bitnamescontacts.h>
#include <sidechain.h>
#include <txdb.h>
#include <uint256.h>
#include <validation.h>

#include <string>

#include <boost/optional.hpp>

BrowsePage::BrowsePage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BrowsePage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    // Table style

#if QT_VERSION < 0x050000
    ui->tableWidgetContacts->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetContacts->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
    ui->tableWidgetContacts->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetContacts->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif

    // Hide vertical header
    ui->tableWidgetContacts->verticalHeader()->setVisible(false);
    // Left align the horizontal header text
    ui->tableWidgetContacts->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    // Set horizontal scroll speed to per 3 pixels
    ui->tableWidgetContacts->horizontalHeader()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    // Select entire row
    ui->tableWidgetContacts->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Select only one row
    ui->tableWidgetContacts->setSelectionMode(QAbstractItemView::SingleSelection);
    // Disable word wrap
    ui->tableWidgetContacts->setWordWrap(false);

    ui->tableWidgetContacts->horizontalHeader()->setStretchLastSection(true);

    Update();
}

BrowsePage::~BrowsePage()
{
    delete ui;
}

void BrowsePage::on_lineEditSearch_returnPressed()
{
    std::string strName = ui->lineEditSearch->text().toStdString();
    if (strName.empty()) {
        QMessageBox::critical(this, tr("Failed to lookup BitName!"),
            tr("You must enter something!\n"),
            QMessageBox::Ok);
        return;
    }

    if (!Resolve(strName)) {
        // TODO message???
    }
}

void BrowsePage::Update()
{
    // List my contacts on table

    ui->tableWidgetContacts->setRowCount(0);

    std::vector<uint256> vContact = bitnamesContacts.GetContacts();

    int nRow = 0;
    for (const uint256& u : vContact) {
        ui->tableWidgetContacts->insertRow(nRow);

        // Get BitNameDB data
        BitName bitname;
        if (!pbitnametree->GetBitName(u, bitname)) {
            return;
        }

        QString name = QString::fromStdString(bitname.strName);

        // Add to table
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ui->tableWidgetContacts->setItem(nRow /* row */, 0 /* col */, nameItem);

        nRow++;
    }
}

void BrowsePage::on_tableWidgetContacts_itemClicked(QTableWidgetItem* item)
{
    if (!item)
        return;

    QString text = item->text();
    if (text.isEmpty())
        return;

    if (!Resolve(text.toStdString())) {
        // TODO message?
    }
}

bool BrowsePage::Resolve(const std::string& strName)
{
    ui->textBrowser->clear();

    BitName bitname;
    if (!pbitnametree->GetBitName(strName, bitname)) {
        QMessageBox::critical(this, tr("Failed to lookup BitName!"),
            tr("BitName not found! Double check the name you have entered and try again!\n"),
            QMessageBox::Ok);
        return false;
    }

    QString result = "";
    result += "Name: " + QString::fromStdString(bitname.strName);
    result += "\nID: " + QString::fromStdString(bitname.nID.ToString());
    result += "\nTxID: " + QString::fromStdString(bitname.txid.front().ToString());

    boost::optional<uint256> commitment = bitname.commitment.front();
    if (commitment)
        result += "\nCommitment: " + QString::fromStdString((*commitment).ToString());

    boost::optional<in_addr_t> opt_in4 = bitname.in4.front();
    if (opt_in4) {
        struct in_addr in4;
        in4.s_addr = *opt_in4;
        result += "\nIPv4: " + QString::fromStdString(std::string(inet_ntoa(in4)));
    }

    ui->textBrowser->setPlainText(result);

    return true;
}
