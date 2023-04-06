// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SWITCHBITNAMESDIALOG_H
#define SWITCHBITNAMESDIALOG_H

#include <QDialog>

namespace Ui {
class SwitchBitNamesDialog;
}

class SwitchBitNamesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SwitchBitNamesDialog(QWidget *parent = nullptr);
    ~SwitchBitNamesDialog();

private:
    Ui::SwitchBitNamesDialog *ui;

    void Update();

private Q_SLOTS:
    void on_pushButtonRegister_clicked();
    void on_pushButtonClaim_clicked();
    void on_pushButtonRecover_clicked();
};

#endif // SWITCHBITNAMESDIALOG_H
