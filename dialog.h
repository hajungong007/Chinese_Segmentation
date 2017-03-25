#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void on_pushButton_clicked();



    void on_parameter1_clicked();

    void on_parameter2_clicked();

    void on_algorithm1_clicked();

    void on_algorithm2_clicked();

    void on_inputfile_clicked();

    void on_pushButton_3_clicked();

    void on_batch_clicked();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
