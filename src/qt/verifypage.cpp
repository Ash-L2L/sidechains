// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/verifypage.h>
#include <qt/forms/ui_verifypage.h>

VerifyPage::VerifyPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VerifyPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
}

VerifyPage::~VerifyPage()
{
    delete ui;
}
