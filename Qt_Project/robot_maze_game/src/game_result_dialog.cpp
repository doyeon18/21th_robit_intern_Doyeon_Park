#include "robot_maze_game/game_result_dialog.h"
#include "ui_game_result_dialog.h"

#include <QStyle>

GameResultDialog::GameResultDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GameResultDialog)
{
    ui->setupUi(this);

    connect(ui->retryButton, &QPushButton::clicked, this, [this] {
        selectedAction_ = Action::Retry;
        accept();
    });
    connect(ui->newMazeButton, &QPushButton::clicked, this, [this] {
        selectedAction_ = Action::NewMaze;
        accept();
    });
    connect(ui->closeButton, &QPushButton::clicked, this, [this] {
        selectedAction_ = Action::Close;
        reject();
    });
}

GameResultDialog::~GameResultDialog()
{
    delete ui;
}

void GameResultDialog::setResult(bool won, const QString &message,
                                 const QString &elapsedTime, int score)
{
    ui->resultBadgeLabel->setText(won ? "MISSION COMPLETE" : "MISSION FAILED");
    ui->resultBadgeLabel->setProperty("result", won ? "won" : "lost");
    ui->resultBadgeLabel->style()->unpolish(ui->resultBadgeLabel);
    ui->resultBadgeLabel->style()->polish(ui->resultBadgeLabel);
    ui->resultTitleLabel->setText(won ? "미로 탈출 성공" : "임무 종료");
    ui->resultMessageLabel->setText(message);
    ui->timeValueLabel->setText(elapsedTime);
    ui->scoreValueLabel->setText(QString::number(score));
}

GameResultDialog::Action GameResultDialog::selectedAction() const
{
    return selectedAction_;
}
