// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/activitypage.h>
#include <qt/forms/ui_activitypage.h>

ActivityPage::ActivityPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ActivityPage)
{
    ui->setupUi(this);
}

ActivityPage::~ActivityPage()
{
    delete ui;
}
