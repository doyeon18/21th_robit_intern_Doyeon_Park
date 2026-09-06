#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QStringList>


// ============================================================
// 생성자
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->inputLineEdit->setReadOnly(true);


    // 문자 확정용 Timer
    inputTimer = new QTimer(this);

    inputTimer->setSingleShot(true);

    inputTimer->setInterval(1000);


    connect(
        inputTimer,
        &QTimer::timeout,
        this,
        &MainWindow::commitCurrentCharacter
        );


    updateButtonLabels();
}


// ============================================================
// 소멸자
// ============================================================

MainWindow::~MainWindow()
{
    delete ui;
}


// ============================================================
// 같은 버튼을 여러 번 눌렀을 때 문자 순환
// ============================================================

void MainWindow::processKey(const QString &key)
{
    if (currentKey == key)
    {
        currentIndex++;

        if (currentIndex >= key.length())
        {
            currentIndex = 0;
        }
    }
    else
    {
        commitCurrentCharacter();

        currentKey = key;
        currentIndex = 0;
    }


    inputTimer->start();

    updateDisplay();
}


// ============================================================
// 현재 선택 중인 문자
// ============================================================

QChar MainWindow::currentCharacter() const
{
    if (currentKey.isEmpty() ||
        currentIndex < 0)
    {
        return QChar();
    }


    QChar ch =
        currentKey.at(currentIndex);


    // 영어일 때만 대소문자 처리
    if (!koreanMode &&
        !symbolMode &&
        ch.isLetter())
    {
        if (uppercase)
        {
            ch = ch.toUpper();
        }
        else
        {
            ch = ch.toLower();
        }
    }


    return ch;
}


// ============================================================
// 현재 선택 중인 문자 확정
// ============================================================

void MainWindow::commitCurrentCharacter()
{
    if (currentKey.isEmpty() ||
        currentIndex < 0)
    {
        return;
    }


    QChar ch =
        currentCharacter();


    // 한글 모드
    if (koreanMode &&
        !symbolMode)
    {
        handleKoreanConsonant(ch);
    }

    // 영문 / 특수문자
    else
    {
        committedText.append(ch);
    }


    currentKey.clear();

    currentIndex = -1;

    inputTimer->stop();


    updateDisplay();
}


// ============================================================
// 화면 갱신
// ============================================================

void MainWindow::updateDisplay()
{
    QString displayText =
        committedText;


    // 한글 조합 중인 글자
    if (koreanMode &&
        !symbolMode)
    {
        displayText +=
            currentKoreanText();
    }


    // 아직 확정되지 않은 버튼 문자
    if (!currentKey.isEmpty() &&
        currentIndex >= 0)
    {
        displayText.append(
            currentCharacter()
            );
    }


    ui->inputLineEdit->setText(
        displayText
        );
}


// ============================================================
// 한글 조합 중인 글자를 문자열로 반환
// ============================================================

QString MainWindow::currentKoreanText() const
{
    QString result;


    // 초성도 없는 경우
    if (cho == -1)
    {
        if (!vowelPattern.isEmpty())
        {
            result += vowelPattern;
        }

        return result;
    }


    // 초성 + 중성이 완성된 경우
    if (jung != -1)
    {
        ushort code =
            0xAC00
            +
            (cho * 21 + jung) * 28
            +
            jong;


        result.append(
            QChar(code)
            );


        return result;
    }


    // 초성만 존재
    result.append(
        choJamoFromIndex(cho)
        );


    // 아직 완성되지 않은 모음
    if (!vowelPattern.isEmpty())
    {
        result += vowelPattern;
    }


    return result;
}


// ============================================================
// 한글 음절 확정
// ============================================================

void MainWindow::commitKoreanSyllable()
{
    if (cho == -1)
    {
        if (!vowelPattern.isEmpty())
        {
            committedText +=
                vowelPattern;
        }
    }

    else if (jung != -1)
    {
        ushort code =
            0xAC00
            +
            (cho * 21 + jung) * 28
            +
            jong;


        committedText.append(
            QChar(code)
            );
    }

    else
    {
        committedText.append(
            choJamoFromIndex(cho)
            );


        if (!vowelPattern.isEmpty())
        {
            committedText +=
                vowelPattern;
        }
    }


    cho = -1;
    jung = -1;
    jong = 0;

    vowelPattern.clear();
}


// ============================================================
// 초성 번호
// ============================================================

int MainWindow::choIndexFromJamo(QChar ch) const
{
    QString choseong =
        QStringLiteral(
            "ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ"
            );


    return choseong.indexOf(ch);
}


// ============================================================
// 종성 번호
// ============================================================

int MainWindow::jongIndexFromJamo(QChar ch) const
{
    // 맨 앞 공백은 종성 없음 = 0
    QString jongseong =
        QStringLiteral(
            " ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ"
            );


    int index =
        jongseong.indexOf(ch);


    if (index <= 0)
    {
        return 0;
    }


    return index;
}


// ============================================================
// 종성을 다음 음절 초성으로 이동
// ============================================================

int MainWindow::jongToCho(
    int jongIndex
    ) const
{
    QString jongseong =
        QStringLiteral(
            " ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ"
            );


    if (jongIndex <= 0 ||
        jongIndex >= jongseong.length())
    {
        return -1;
    }


    QChar ch =
        jongseong.at(jongIndex);


    return choIndexFromJamo(ch);
}


// ============================================================
// 초성 번호 -> 자모
// ============================================================

QChar MainWindow::choJamoFromIndex(
    int index
    ) const
{
    QString choseong =
        QStringLiteral(
            "ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ"
            );


    if (index < 0 ||
        index >= choseong.length())
    {
        return QChar();
    }


    return choseong.at(index);
}


// ============================================================
// 한글 자음 입력
// ============================================================

void MainWindow::handleKoreanConsonant(
    QChar ch
    )
{
    int newCho =
        choIndexFromJamo(ch);


    if (newCho == -1)
        return;


    // 아무것도 없는 상태
    if (cho == -1)
    {
        cho = newCho;

        updateDisplay();

        return;
    }


    // 초성만 있는 상태
    if (jung == -1)
    {
        commitKoreanSyllable();

        cho = newCho;

        updateDisplay();

        return;
    }


    // 초성 + 중성 상태
    // → 자음을 받침으로 사용
    if (jong == 0)
    {
        int newJong =
            jongIndexFromJamo(ch);


        if (newJong != 0)
        {
            jong = newJong;
        }
        else
        {
            commitKoreanSyllable();

            cho = newCho;
        }


        updateDisplay();

        return;
    }


    // 이미 받침까지 존재하면
    // 기존 음절 확정 후 새 초성
    commitKoreanSyllable();

    cho = newCho;


    updateDisplay();
}


// ============================================================
// 천지인 모음 패턴 -> 중성 번호
// ============================================================

int MainWindow::vowelIndexFromPattern(
    const QString &pattern
    ) const
{
    // 기본 모음

    if (pattern == QStringLiteral("ㅣ"))
        return 20;     // ㅣ

    if (pattern == QStringLiteral("ㅡ"))
        return 18;     // ㅡ


    if (pattern == QStringLiteral("ㅣㆍ"))
        return 0;      // ㅏ

    if (pattern == QStringLiteral("ㅣㆍㆍ"))
        return 2;      // ㅑ


    if (pattern == QStringLiteral("ㆍㅣ"))
        return 4;      // ㅓ

    if (pattern == QStringLiteral("ㆍㆍㅣ"))
        return 6;      // ㅕ


    if (pattern == QStringLiteral("ㆍㅡ"))
        return 8;      // ㅗ

    if (pattern == QStringLiteral("ㆍㆍㅡ"))
        return 12;     // ㅛ


    if (pattern == QStringLiteral("ㅡㆍ"))
        return 13;     // ㅜ

    if (pattern == QStringLiteral("ㅡㆍㆍ"))
        return 17;     // ㅠ


    // 복합 모음

    if (pattern == QStringLiteral("ㅣㆍㅣ"))
        return 1;      // ㅐ

    if (pattern == QStringLiteral("ㅣㆍㆍㅣ"))
        return 3;      // ㅒ


    if (pattern == QStringLiteral("ㆍㅣㅣ"))
        return 5;      // ㅔ

    if (pattern == QStringLiteral("ㆍㆍㅣㅣ"))
        return 7;      // ㅖ


    if (pattern == QStringLiteral("ㆍㅡㅣㆍ"))
        return 9;      // ㅘ

    if (pattern == QStringLiteral("ㆍㅡㅣㆍㅣ"))
        return 10;     // ㅙ

    if (pattern == QStringLiteral("ㆍㅡㅣ"))
        return 11;     // ㅚ


    if (pattern == QStringLiteral("ㅡㆍㆍㅣ"))
        return 14;     // ㅝ

    if (pattern == QStringLiteral("ㅡㆍㆍㅣㅣ"))
        return 15;     // ㅞ

    if (pattern == QStringLiteral("ㅡㆍㅣ"))
        return 16;     // ㅟ


    if (pattern == QStringLiteral("ㅡㅣ"))
        return 19;     // ㅢ


    return -1;
}


// ============================================================
// 현재 모음 입력이 유효한 천지인 조합의 시작인지 확인
// ============================================================

bool MainWindow::isVowelPatternPrefix(
    const QString &pattern
    ) const
{
    static const QStringList patterns =
        {
            QStringLiteral("ㅣ"),
            QStringLiteral("ㅡ"),

            QStringLiteral("ㅣㆍ"),
            QStringLiteral("ㅣㆍㆍ"),

            QStringLiteral("ㆍㅣ"),
            QStringLiteral("ㆍㆍㅣ"),

            QStringLiteral("ㆍㅡ"),
            QStringLiteral("ㆍㆍㅡ"),

            QStringLiteral("ㅡㆍ"),
            QStringLiteral("ㅡㆍㆍ"),

            QStringLiteral("ㅣㆍㅣ"),
            QStringLiteral("ㅣㆍㆍㅣ"),

            QStringLiteral("ㆍㅣㅣ"),
            QStringLiteral("ㆍㆍㅣㅣ"),

            QStringLiteral("ㆍㅡㅣ"),
            QStringLiteral("ㆍㅡㅣㆍ"),
            QStringLiteral("ㆍㅡㅣㆍㅣ"),

            QStringLiteral("ㅡㆍㅣ"),
            QStringLiteral("ㅡㆍㆍㅣ"),
            QStringLiteral("ㅡㆍㆍㅣㅣ"),

            QStringLiteral("ㅡㅣ")
        };


    for (const QString &valid :
         patterns)
    {
        if (valid.startsWith(pattern))
        {
            return true;
        }
    }


    return false;
}


// ============================================================
// 천지인 모음 획 입력
// ============================================================

void MainWindow::handleKoreanVowelStroke(
    const QString &stroke
    )
{
    // 받침이 있는 상태에서 모음이 오면
    // 받침을 다음 음절 초성으로 넘김
    if (jong != 0)
    {
        int nextCho =
            jongToCho(jong);


        // 기존 음절에서는 받침 제거
        jong = 0;


        commitKoreanSyllable();


        // 기존 받침을 새 초성으로 사용
        if (nextCho != -1)
        {
            cho = nextCho;
        }
        else
        {
            cho = 11;  // ㅇ
        }
    }


    // 초성이 없는 상태에서 모음부터 입력하면
    // 자동으로 ㅇ 사용
    if (cho == -1)
    {
        cho = 11;
    }


    // 현재 모음 입력이 없는 경우
    if (vowelPattern.isEmpty())
    {
        vowelPattern = stroke;

        jung =
            vowelIndexFromPattern(
                vowelPattern
                );


        updateDisplay();

        return;
    }


    QString candidate =
        vowelPattern + stroke;


    // 현재 입력을 이어 붙여도
    // 유효한 천지인 모음 조합인 경우
    if (isVowelPatternPrefix(candidate))
    {
        vowelPattern =
            candidate;


        int newJung =
            vowelIndexFromPattern(
                vowelPattern
                );


        if (newJung != -1)
        {
            jung = newJung;
        }
    }

    // 더 이상 같은 모음으로 결합할 수 없는 경우
    else
    {
        commitKoreanSyllable();


        // 모음으로 새 음절 시작
        cho = 11;


        vowelPattern =
            stroke;


        jung =
            vowelIndexFromPattern(
                vowelPattern
                );
    }


    updateDisplay();
}


// ============================================================
// 첫 번째 버튼
//
// 영어 : .?!
// 한글 : ㅣ
// 숫자 : 1 . ? !
// ============================================================

void MainWindow::on_punctuationButton_clicked()
{
    if (symbolMode)
    {
        processKey("1.,?!");
    }
    else if (koreanMode)
    {
        commitCurrentCharacter();

        handleKoreanVowelStroke(
            QStringLiteral("ㅣ")
            );
    }
    else
    {
        processKey(".?!");
    }
}


// ============================================================
// ABC / ㆍ / 2@
// ============================================================

void MainWindow::on_abcButton_clicked()
{
    if (symbolMode)
    {
        processKey("2@");
    }
    else if (koreanMode)
    {
        commitCurrentCharacter();

        handleKoreanVowelStroke(
            QStringLiteral("ㆍ")
            );
    }
    else
    {
        processKey("ABC");
    }
}


// ============================================================
// DEF / ㅡ / 3#
// ============================================================

void MainWindow::on_defButton_clicked()
{
    if (symbolMode)
    {
        processKey("3#");
    }
    else if (koreanMode)
    {
        commitCurrentCharacter();

        handleKoreanVowelStroke(
            QStringLiteral("ㅡ")
            );
    }
    else
    {
        processKey("DEF");
    }
}


// ============================================================
// GHI / ㄱㅋ / 4$
// ============================================================

void MainWindow::on_ghiButton_clicked()
{
    if (symbolMode)
    {
        processKey("4$");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄱㅋ")
            );
    }
    else
    {
        processKey("GHI");
    }
}


// ============================================================
// JKL / ㄴㄹ / 5%
// ============================================================

void MainWindow::on_jklButton_clicked()
{
    if (symbolMode)
    {
        processKey("5%");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄴㄹ")
            );
    }
    else
    {
        processKey("JKL");
    }
}


// ============================================================
// MNO / ㄷㅌ / 6&
// ============================================================

void MainWindow::on_mnoButton_clicked()
{
    if (symbolMode)
    {
        processKey("6&");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄷㅌ")
            );
    }
    else
    {
        processKey("MNO");
    }
}


// ============================================================
// PQRS / ㅂㅍ / 7*
// ============================================================

void MainWindow::on_pqrsButton_clicked()
{
    if (symbolMode)
    {
        processKey("7*");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅂㅍ")
            );
    }
    else
    {
        processKey("PQRS");
    }
}


// ============================================================
// TUV / ㅅㅎ / 8(
// ============================================================

void MainWindow::on_tuvButton_clicked()
{
    if (symbolMode)
    {
        processKey("8(");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅅㅎ")
            );
    }
    else
    {
        processKey("TUV");
    }
}


// ============================================================
// WXYZ / ㅈㅊ / 9)
// ============================================================

void MainWindow::on_wxyzButton_clicked()
{
    if (symbolMode)
    {
        processKey("9)");
    }
    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅈㅊ")
            );
    }
    else
    {
        processKey("WXYZ");
    }
}


// ============================================================
// Shift
//
// 영어 : 대소문자
// 한글 : ㅇ / ㅁ 버튼
// ============================================================

void MainWindow::on_shiftButton_clicked()
{
    if (symbolMode)
    {
        return;
    }


    // 한글 모드에서는 ㅇㅁ 버튼
    if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅇㅁ")
            );

        return;
    }


    // 영어 모드에서는 Shift
    commitCurrentCharacter();

    uppercase = !uppercase;

    updateButtonLabels();
}


// ============================================================
// 한 / 영
// ============================================================

void MainWindow::on_languageButton_clicked()
{
    commitCurrentCharacter();


    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    // 숫자모드 종료
    symbolMode = false;


    // 한 / 영 전환
    koreanMode =
        !koreanMode;


    updateButtonLabels();

    updateDisplay();
}


// ============================================================
// 숫자 / 특수문자 전환
// ============================================================

void MainWindow::on_symbolButton_clicked()
{
    commitCurrentCharacter();


    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    symbolMode =
        !symbolMode;


    updateButtonLabels();

    updateDisplay();
}


// ============================================================
// Space
// ============================================================

void MainWindow::on_spaceButton_clicked()
{
    // 숫자 모드
    if (symbolMode)
    {
        commitCurrentCharacter();

        committedText.append('0');

        updateDisplay();

        return;
    }


    commitCurrentCharacter();


    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    committedText.append(' ');


    updateDisplay();
}


// ============================================================
// Backspace
// ============================================================

void MainWindow::on_backspaceButton_clicked()
{
    // 아직 선택 중인 문자
    if (!currentKey.isEmpty())
    {
        currentKey.clear();

        currentIndex = -1;

        inputTimer->stop();


        updateDisplay();

        return;
    }


    // 한글 조합 중인 글자가 있다면
    if (koreanMode &&
        (cho != -1 ||
         !vowelPattern.isEmpty()))
    {
        cho = -1;
        jung = -1;
        jong = 0;

        vowelPattern.clear();


        updateDisplay();

        return;
    }


    // 완성된 문자열 마지막 글자 삭제
    if (!committedText.isEmpty())
    {
        committedText.chop(1);
    }


    updateDisplay();
}


// ============================================================
// 버튼 표시 변경
// ============================================================

void MainWindow::updateButtonLabels()
{
    // ========================================================
    // 숫자 / 특수문자
    // ========================================================

    if (symbolMode)
    {
        ui->punctuationButton->setText("1 . ? !");
        ui->abcButton->setText("2 @");
        ui->defButton->setText("3 #");

        ui->ghiButton->setText("4 $");
        ui->jklButton->setText("5 %");
        ui->mnoButton->setText("6 &");

        ui->pqrsButton->setText("7 *");
        ui->tuvButton->setText("8 (");
        ui->wxyzButton->setText("9 )");

        ui->spaceButton->setText("0");

        ui->shiftButton->setText("-");

        ui->symbolButton->setText("문자");

        return;
    }


    // ========================================================
    // 한글
    // ========================================================

    if (koreanMode)
    {
        ui->punctuationButton->setText("ㅣ");
        ui->abcButton->setText("ㆍ");
        ui->defButton->setText("ㅡ");

        ui->ghiButton->setText("ㄱㅋ");
        ui->jklButton->setText("ㄴㄹ");
        ui->mnoButton->setText("ㄷㅌ");

        ui->pqrsButton->setText("ㅂㅍ");
        ui->tuvButton->setText("ㅅㅎ");
        ui->wxyzButton->setText("ㅈㅊ");

        ui->shiftButton->setText("ㅇㅁ");

        ui->spaceButton->setText("Space");

        ui->symbolButton->setText("!#1");

        ui->languageButton->setText("한/영");

        return;
    }


    // ========================================================
    // 영어 대문자
    // ========================================================

    if (uppercase)
    {
        ui->punctuationButton->setText(".?!");

        ui->abcButton->setText("ABC");
        ui->defButton->setText("DEF");

        ui->ghiButton->setText("GHI");
        ui->jklButton->setText("JKL");
        ui->mnoButton->setText("MNO");

        ui->pqrsButton->setText("PQRS");
        ui->tuvButton->setText("TUV");
        ui->wxyzButton->setText("WXYZ");

        ui->shiftButton->setText("Shift ↑");
    }

    // ========================================================
    // 영어 소문자
    // ========================================================

    else
    {
        ui->punctuationButton->setText(".?!");

        ui->abcButton->setText("abc");
        ui->defButton->setText("def");

        ui->ghiButton->setText("ghi");
        ui->jklButton->setText("jkl");
        ui->mnoButton->setText("mno");

        ui->pqrsButton->setText("pqrs");
        ui->tuvButton->setText("tuv");
        ui->wxyzButton->setText("wxyz");

        ui->shiftButton->setText("Shift ↓");
    }


    ui->spaceButton->setText("Space");

    ui->symbolButton->setText("!#1");

    ui->languageButton->setText("한/영");
}


// ============================================================
// Enter
// ============================================================

void MainWindow::on_enterButton_clicked()
{
    commitCurrentCharacter();


    // 한글 작성 중이라면 확정
    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    if (committedText.isEmpty())
    {
        return;
    }


    QString filePath =
        QDir::homePath()
        +
        "/QtStudy/day03/hw3/output.txt";


    QFile file(filePath);


    // 파일 마지막 줄에 추가
    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Append |
            QIODevice::Text))
    {
        QMessageBox::warning(
            this,
            "File Error",
            "output.txt 파일을 열 수 없습니다."
            );

        return;
    }


    QTextStream out(&file);


    out
        << committedText
        << '\n';


    file.close();


    // 입력 초기화
    committedText.clear();

    currentKey.clear();

    currentIndex = -1;


    cho = -1;
    jung = -1;
    jong = 0;

    vowelPattern.clear();


    updateDisplay();
}
