#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->speedSlider,
            &QSlider::valueChanged,
            this,
            &MainWindow::updateSpeed);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_forwardButton_clicked()
{
    ui->statusLabel->setText("현재 상태 : 전진");
}

void MainWindow::on_backwardButton_clicked()
{
    ui->statusLabel->setText("현재 상태 : 후진");
}

void MainWindow::on_leftButton_clicked()
{
    ui->statusLabel->setText("현재 상태 : 좌회전");
}

void MainWindow::on_rightButton_clicked()
{
    ui->statusLabel->setText("현재 상태 : 우회전");
}

void MainWindow::on_stopButton_clicked()
{
    ui->statusLabel->setText("현재 상태 : 정지");
}

void MainWindow::updateSpeed(int value)
{
    ui->speedLabel->setText("속도 : " + QString::number(value));
}
