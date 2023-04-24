// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RECOVERBITNAMEDIALOG_H
#define RECOVERBITNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class RecoverBitNameDialog;
}

class RecoverBitNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecoverBitNameDialog(QWidget *parent = nullptr);
    ~RecoverBitNameDialog();

private:
    Ui::RecoverBitNameDialog *ui;
};

#endif // RECOVERBITNAMEDIALOG_H
