// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIFYPAGE_H
#define VERIFYPAGE_H

#include <QWidget>

class PlatformStyle;

namespace Ui {
class VerifyPage;
}

class VerifyPage : public QWidget
{
    Q_OBJECT

public:
    explicit VerifyPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~VerifyPage();

private:
    Ui::VerifyPage *ui;

    const PlatformStyle *platformStyle;
};

#endif // VERIFYPAGE_H
