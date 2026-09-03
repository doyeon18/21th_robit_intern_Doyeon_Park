#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_forwardButton_clicked();
    void on_backwardButton_clicked();
    void on_leftButton_clicked();
    void on_rightButton_clicked();
    void on_stopButton_clicked();

    void updateSpeed(int value);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
