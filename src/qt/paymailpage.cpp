// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/paymailpage.h>
#include <qt/forms/ui_paymailpage.h>

PaymailPage::PaymailPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PaymailPage)
{
    ui->setupUi(this);
}

PaymailPage::~PaymailPage()
{
    delete ui;
}
