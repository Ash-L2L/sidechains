// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/browsepage.h>
#include <qt/forms/ui_browsepage.h>

#include <qt/platformstyle.h>

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
