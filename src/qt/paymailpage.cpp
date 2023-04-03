// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/paymailpage.h>
#include <qt/forms/ui_paymailpage.h>

#include <qt/platformstyle.h>

PaymailPage::PaymailPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PaymailPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
}

PaymailPage::~PaymailPage()
{
    delete ui;
}
