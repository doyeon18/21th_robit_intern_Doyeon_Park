#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <QChar>

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
    void on_punctuationButton_clicked();

    void on_abcButton_clicked();
    void on_defButton_clicked();
    void on_ghiButton_clicked();
    void on_jklButton_clicked();
    void on_mnoButton_clicked();
    void on_pqrsButton_clicked();
    void on_tuvButton_clicked();
    void on_wxyzButton_clicked();

    void on_backspaceButton_clicked();
    void on_enterButton_clicked();
    void on_shiftButton_clicked();
    void on_spaceButton_clicked();

    void on_symbolButton_clicked();
    void on_languageButton_clicked();

    void commitCurrentCharacter();

private:
    Ui::MainWindow *ui;


    // =========================================================
    // 공통 입력 상태
    // =========================================================

    QString committedText;

    QString currentKey;

    int currentIndex = -1;

    QTimer *inputTimer;


    // =========================================================
    // 영문 상태
    // =========================================================

    bool uppercase = true;


    // =========================================================
    // 입력 모드
    // =========================================================

    bool symbolMode = false;
    bool koreanMode = false;


    // =========================================================
    // 한글 조합 상태
    //
    // cho  : 초성
    // jung : 중성
    // jong : 종성
    // =========================================================

    int cho = -1;
    int jung = -1;
    int jong = 0;

    // 천지인 모음 입력 순서 저장
    QString vowelPattern;


    // =========================================================
    // 공통 입력 함수
    // =========================================================

    void processKey(const QString &key);

    QChar currentCharacter() const;

    void updateDisplay();

    void updateButtonLabels();


    // =========================================================
    // 한글 처리
    // =========================================================

    void handleKoreanConsonant(QChar ch);

    void handleKoreanVowelStroke(const QString &stroke);

    void commitKoreanSyllable();

    QString currentKoreanText() const;


    // 초성 / 종성 번호 찾기
    int choIndexFromJamo(QChar ch) const;

    int jongIndexFromJamo(QChar ch) const;

    int jongToCho(int jongIndex) const;

    QChar choJamoFromIndex(int index) const;


    // 천지인 모음 패턴
    int vowelIndexFromPattern(
        const QString &pattern
        ) const;

    bool isVowelPatternPrefix(
        const QString &pattern
        ) const;
};

#endif
