// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CLAIMBITNAMEDIALOG_H
#define CLAIMBITNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class ClaimBitNameDialog;
}

class ClaimBitNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClaimBitNameDialog(QWidget *parent = nullptr);
    ~ClaimBitNameDialog();

private:
    Ui::ClaimBitNameDialog *ui;
};

#endif // CLAIMBITNAMEDIALOG_H
