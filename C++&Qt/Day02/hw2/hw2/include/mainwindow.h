#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFrame>
#include <vector>

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
    void updateCar();

    void on_forwardButton_pressed();
    void on_forwardButton_released();

    void on_backwardButton_pressed();
    void on_backwardButton_released();

    void on_leftButton_pressed();
    void on_leftButton_released();

    void on_rightButton_pressed();
    void on_rightButton_released();

    void on_stopButton_clicked();

    void on_undoButton_clicked();
    void on_redoButton_clicked();

private:
    struct CarState
    {
        double x;
        double y;
        double angle;
    };

    enum DriveMode
    {
        Stop,
        Forward,
        Backward,
        TurnLeft,
        TurnRight
    };

    Ui::MainWindow *ui;

    QTimer *timer;
    QFrame *directionDot;

    std::vector<CarState> historyStack;
    std::vector<CarState> redoStack;

    double carX;
    double carY;
    double carAngle;

    DriveMode driveMode;
    bool movementSessionActive;

    void startMovement(DriveMode mode);
    void finishMovement();

    CarState currentState() const;
    void restoreState(const CarState &state);

    void updateDirectionDot();
    void updateLabels();
};

#endif // MAINWINDOW_H
