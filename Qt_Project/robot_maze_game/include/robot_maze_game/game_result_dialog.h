#ifndef GAME_RESULT_DIALOG_H
#define GAME_RESULT_DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class GameResultDialog;
}
QT_END_NAMESPACE

class GameResultDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Action
    {
        Close,
        Retry,
        NewMaze
    };

    explicit GameResultDialog(QWidget *parent = nullptr);
    ~GameResultDialog() override;

    void setResult(bool won, const QString &message,
                   const QString &elapsedTime, int score);
    Action selectedAction() const;

private:
    Ui::GameResultDialog *ui;
    Action selectedAction_ = Action::Close;
};

#endif // GAME_RESULT_DIALOG_H
