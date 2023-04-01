// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PAYMAILPAGE_H
#define PAYMAILPAGE_H

#include <QWidget>

namespace Ui {
class PaymailPage;
}

class PaymailPage : public QWidget
{
    Q_OBJECT

public:
    explicit PaymailPage(QWidget *parent = nullptr);
    ~PaymailPage();

private:
    Ui::PaymailPage *ui;
};

#endif // PAYMAILPAGE_H
