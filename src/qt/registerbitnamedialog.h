// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REGISTERBITNAMEDIALOG_H
#define REGISTERBITNAMEDIALOG_H

#include <QDialog>

namespace Ui {
class RegisterBitNameDialog;
}

class RegisterBitNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterBitNameDialog(QWidget *parent = nullptr);
    ~RegisterBitNameDialog();

private:
    Ui::RegisterBitNameDialog *ui;

private Q_SLOTS:
    void on_pushButtonReserve_clicked();
    void on_pushButtonRegister_clicked();
};

#endif // REGISTERBITNAMEDIALOG_H
