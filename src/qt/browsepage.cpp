// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/browsepage.h>
#include <qt/forms/ui_browsepage.h>

#include <qt/platformstyle.h>

#include <QMessageBox>

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
}

BrowsePage::~BrowsePage()
{
    delete ui;
}

void BrowsePage::on_lineEditSearch_returnPressed()
{
    ui->textBrowser->clear();

    std::string strName = ui->lineEditSearch->text().toStdString();
    if (strName.empty()) {
        QMessageBox::critical(this, tr("Failed to lookup BitName!"),
            tr("You must enter something!\n"),
            QMessageBox::Ok);
        return;
    }

    BitName bitname;
    if (!pbitnametree->GetBitName(strName, bitname)) {
        QMessageBox::critical(this, tr("Failed to lookup BitName!"),
            tr("BitName not found! Double check the name you have entered and try again!\n"),
            QMessageBox::Ok);
        return;
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
}
