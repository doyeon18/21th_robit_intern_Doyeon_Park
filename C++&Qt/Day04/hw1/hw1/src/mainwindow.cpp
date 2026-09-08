#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStringList>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QByteArray>


// ============================================================
// 생성자
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    ui->inputLineEdit->setReadOnly(true);

    ui->chatTextEdit->setReadOnly(true);


    // ========================================================
    // UDP 설정
    // ========================================================

    udpSocket =
        new QUdpSocket(this);


    ui->localPortLineEdit->setText(
        "10000"
        );


    ui->peerIpLineEdit->setText(
        "172.100.0.183"
        );


    ui->peerPortLineEdit->setText(
        "10000"
        );


    connect(
        udpSocket,
        &QUdpSocket::readyRead,
        this,
        &MainWindow::udpRead
        );


    // ========================================================
    // 문자 확정 Timer
    // ========================================================

    inputTimer =
        new QTimer(this);


    inputTimer->setSingleShot(
        true
        );


    inputTimer->setInterval(
        1000
        );


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

void MainWindow::processKey(
    const QString &key
    )
{
    if (currentKey == key)
    {
        currentIndex++;


        if (currentIndex >=
            key.length())
        {
            currentIndex = 0;
        }
    }

    else
    {
        commitCurrentCharacter();


        currentKey =
            key;


        currentIndex =
            0;
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
        currentKey.at(
            currentIndex
            );


    if (!koreanMode &&
        !symbolMode &&
        ch.isLetter())
    {
        if (uppercase)
        {
            ch =
                ch.toUpper();
        }

        else
        {
            ch =
                ch.toLower();
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


    if (koreanMode &&
        !symbolMode)
    {
        handleKoreanConsonant(
            ch
            );
    }

    else
    {
        committedText.append(
            ch
            );
    }


    currentKey.clear();


    currentIndex =
        -1;


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


    if (koreanMode &&
        !symbolMode)
    {
        displayText +=
            currentKoreanText();
    }


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


    if (cho == -1)
    {
        if (!vowelPattern.isEmpty())
        {
            result +=
                vowelPattern;
        }


        return result;
    }


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


    result.append(
        choJamoFromIndex(
            cho
            )
        );


    if (!vowelPattern.isEmpty())
    {
        result +=
            vowelPattern;
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
            choJamoFromIndex(
                cho
                )
            );


        if (!vowelPattern.isEmpty())
        {
            committedText +=
                vowelPattern;
        }
    }


    cho =
        -1;


    jung =
        -1;


    jong =
        0;


    vowelPattern.clear();
}


// ============================================================
// 초성 번호
// ============================================================

int MainWindow::choIndexFromJamo(
    QChar ch
    ) const
{
    QString choseong =
        QStringLiteral(
            "ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ"
            );


    return choseong.indexOf(
        ch
        );
}


// ============================================================
// 종성 번호
// ============================================================

int MainWindow::jongIndexFromJamo(
    QChar ch
    ) const
{
    QString jongseong =
        QStringLiteral(
            " ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ"
            );


    int index =
        jongseong.indexOf(
            ch
            );


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
        jongIndex >=
            jongseong.length())
    {
        return -1;
    }


    QChar ch =
        jongseong.at(
            jongIndex
            );


    return choIndexFromJamo(
        ch
        );
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
        index >=
            choseong.length())
    {
        return QChar();
    }


    return choseong.at(
        index
        );
}


// ============================================================
// 한글 자음 입력
// ============================================================

void MainWindow::handleKoreanConsonant(
    QChar ch
    )
{
    int newCho =
        choIndexFromJamo(
            ch
            );


    if (newCho == -1)
    {
        return;
    }


    if (cho == -1)
    {
        cho =
            newCho;


        updateDisplay();


        return;
    }


    if (jung == -1)
    {
        commitKoreanSyllable();


        cho =
            newCho;


        updateDisplay();


        return;
    }


    if (jong == 0)
    {
        int newJong =
            jongIndexFromJamo(
                ch
                );


        if (newJong != 0)
        {
            jong =
                newJong;
        }

        else
        {
            commitKoreanSyllable();


            cho =
                newCho;
        }


        updateDisplay();


        return;
    }


    commitKoreanSyllable();


    cho =
        newCho;


    updateDisplay();
}


// ============================================================
// 천지인 모음 패턴 -> 중성 번호
// ============================================================

int MainWindow::vowelIndexFromPattern(
    const QString &pattern
    ) const
{
    if (pattern ==
        QStringLiteral("ㅣ"))
    {
        return 20;
    }


    if (pattern ==
        QStringLiteral("ㅡ"))
    {
        return 18;
    }


    if (pattern ==
        QStringLiteral("ㅣㆍ"))
    {
        return 0;
    }


    if (pattern ==
        QStringLiteral("ㅣㆍㆍ"))
    {
        return 2;
    }


    if (pattern ==
        QStringLiteral("ㆍㅣ"))
    {
        return 4;
    }


    if (pattern ==
        QStringLiteral("ㆍㆍㅣ"))
    {
        return 6;
    }


    if (pattern ==
        QStringLiteral("ㆍㅡ"))
    {
        return 8;
    }


    if (pattern ==
        QStringLiteral("ㆍㆍㅡ"))
    {
        return 12;
    }


    if (pattern ==
        QStringLiteral("ㅡㆍ"))
    {
        return 13;
    }


    if (pattern ==
        QStringLiteral("ㅡㆍㆍ"))
    {
        return 17;
    }


    if (pattern ==
        QStringLiteral("ㅣㆍㅣ"))
    {
        return 1;
    }


    if (pattern ==
        QStringLiteral("ㅣㆍㆍㅣ"))
    {
        return 3;
    }


    if (pattern ==
        QStringLiteral("ㆍㅣㅣ"))
    {
        return 5;
    }


    if (pattern ==
        QStringLiteral("ㆍㆍㅣㅣ"))
    {
        return 7;
    }


    if (pattern ==
        QStringLiteral("ㆍㅡㅣㆍ"))
    {
        return 9;
    }


    if (pattern ==
        QStringLiteral("ㆍㅡㅣㆍㅣ"))
    {
        return 10;
    }


    if (pattern ==
        QStringLiteral("ㆍㅡㅣ"))
    {
        return 11;
    }


    if (pattern ==
        QStringLiteral("ㅡㆍㆍㅣ"))
    {
        return 14;
    }


    if (pattern ==
        QStringLiteral("ㅡㆍㆍㅣㅣ"))
    {
        return 15;
    }


    if (pattern ==
        QStringLiteral("ㅡㆍㅣ"))
    {
        return 16;
    }


    if (pattern ==
        QStringLiteral("ㅡㅣ"))
    {
        return 19;
    }


    return -1;
}


// ============================================================
// 유효한 천지인 모음 조합의 시작인지 확인
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
        if (valid.startsWith(
                pattern
                ))
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
    if (jong != 0)
    {
        int nextCho =
            jongToCho(
                jong
                );


        jong =
            0;


        commitKoreanSyllable();


        if (nextCho != -1)
        {
            cho =
                nextCho;
        }

        else
        {
            cho =
                11;
        }
    }


    if (cho == -1)
    {
        cho =
            11;
    }


    if (vowelPattern.isEmpty())
    {
        vowelPattern =
            stroke;


        jung =
            vowelIndexFromPattern(
                vowelPattern
                );


        updateDisplay();


        return;
    }


    QString candidate =
        vowelPattern
        +
        stroke;


    if (isVowelPatternPrefix(
            candidate
            ))
    {
        vowelPattern =
            candidate;


        int newJung =
            vowelIndexFromPattern(
                vowelPattern
                );


        if (newJung != -1)
        {
            jung =
                newJung;
        }
    }

    else
    {
        commitKoreanSyllable();


        cho =
            11;


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
// .?! / ㅣ / 1
// ============================================================

void MainWindow::on_punctuationButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "1.,?!"
            );
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
        processKey(
            ".?!"
            );
    }
}


// ============================================================
// ABC / ㆍ / 2
// ============================================================

void MainWindow::on_abcButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "2@"
            );
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
        processKey(
            "ABC"
            );
    }
}


// ============================================================
// DEF / ㅡ / 3
// ============================================================

void MainWindow::on_defButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "3#"
            );
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
        processKey(
            "DEF"
            );
    }
}


// ============================================================
// GHI / ㄱㅋ / 4
// ============================================================

void MainWindow::on_ghiButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "4$"
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄱㅋ")
            );
    }

    else
    {
        processKey(
            "GHI"
            );
    }
}


// ============================================================
// JKL / ㄴㄹ / 5
// ============================================================

void MainWindow::on_jklButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "5%"
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄴㄹ")
            );
    }

    else
    {
        processKey(
            "JKL"
            );
    }
}


// ============================================================
// MNO / ㄷㅌ / 6
// ============================================================

void MainWindow::on_mnoButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "6&"
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㄷㅌ")
            );
    }

    else
    {
        processKey(
            "MNO"
            );
    }
}


// ============================================================
// PQRS / ㅂㅍ / 7
// ============================================================

void MainWindow::on_pqrsButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "7*"
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅂㅍ")
            );
    }

    else
    {
        processKey(
            "PQRS"
            );
    }
}


// ============================================================
// TUV / ㅅㅎ / 8
// ============================================================

void MainWindow::on_tuvButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "8("
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅅㅎ")
            );
    }

    else
    {
        processKey(
            "TUV"
            );
    }
}


// ============================================================
// WXYZ / ㅈㅊ / 9
// ============================================================

void MainWindow::on_wxyzButton_clicked()
{
    if (symbolMode)
    {
        processKey(
            "9)"
            );
    }

    else if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅈㅊ")
            );
    }

    else
    {
        processKey(
            "WXYZ"
            );
    }
}


// ============================================================
// Shift
// ============================================================

void MainWindow::on_shiftButton_clicked()
{
    if (symbolMode)
    {
        return;
    }


    if (koreanMode)
    {
        processKey(
            QStringLiteral("ㅇㅁ")
            );


        return;
    }


    commitCurrentCharacter();


    uppercase =
        !uppercase;


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


    symbolMode =
        false;


    koreanMode =
        !koreanMode;


    updateButtonLabels();


    updateDisplay();
}


// ============================================================
// 숫자 / 특수문자
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
    if (symbolMode)
    {
        commitCurrentCharacter();


        committedText.append(
            '0'
            );


        updateDisplay();


        return;
    }


    commitCurrentCharacter();


    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    committedText.append(
        ' '
        );


    updateDisplay();
}


// ============================================================
// Backspace
// ============================================================

void MainWindow::on_backspaceButton_clicked()
{
    if (!currentKey.isEmpty())
    {
        currentKey.clear();


        currentIndex =
            -1;


        inputTimer->stop();


        updateDisplay();


        return;
    }


    if (koreanMode &&
        (cho != -1 ||
         !vowelPattern.isEmpty()))
    {
        cho =
            -1;


        jung =
            -1;


        jong =
            0;


        vowelPattern.clear();


        updateDisplay();


        return;
    }


    if (!committedText.isEmpty())
    {
        committedText.chop(
            1
            );
    }


    updateDisplay();
}


// ============================================================
// 버튼 표시 변경
// ============================================================

void MainWindow::updateButtonLabels()
{
    if (symbolMode)
    {
        ui->punctuationButton->setText(
            "1 . ? !"
            );

        ui->abcButton->setText(
            "2 @"
            );

        ui->defButton->setText(
            "3 #"
            );


        ui->ghiButton->setText(
            "4 $"
            );

        ui->jklButton->setText(
            "5 %"
            );

        ui->mnoButton->setText(
            "6 &"
            );


        ui->pqrsButton->setText(
            "7 *"
            );

        ui->tuvButton->setText(
            "8 ("
            );

        ui->wxyzButton->setText(
            "9 )"
            );


        ui->spaceButton->setText(
            "0"
            );


        ui->shiftButton->setText(
            "-"
            );


        ui->symbolButton->setText(
            QStringLiteral("문자")
            );


        return;
    }


    if (koreanMode)
    {
        ui->punctuationButton->setText(
            QStringLiteral("ㅣ")
            );

        ui->abcButton->setText(
            QStringLiteral("ㆍ")
            );

        ui->defButton->setText(
            QStringLiteral("ㅡ")
            );


        ui->ghiButton->setText(
            QStringLiteral("ㄱㅋ")
            );

        ui->jklButton->setText(
            QStringLiteral("ㄴㄹ")
            );

        ui->mnoButton->setText(
            QStringLiteral("ㄷㅌ")
            );


        ui->pqrsButton->setText(
            QStringLiteral("ㅂㅍ")
            );

        ui->tuvButton->setText(
            QStringLiteral("ㅅㅎ")
            );

        ui->wxyzButton->setText(
            QStringLiteral("ㅈㅊ")
            );


        ui->shiftButton->setText(
            QStringLiteral("ㅇㅁ")
            );


        ui->spaceButton->setText(
            "Space"
            );


        ui->symbolButton->setText(
            "!#1"
            );


        ui->languageButton->setText(
            QStringLiteral("한/영")
            );


        return;
    }


    if (uppercase)
    {
        ui->punctuationButton->setText(
            ".?!"
            );


        ui->abcButton->setText(
            "ABC"
            );

        ui->defButton->setText(
            "DEF"
            );


        ui->ghiButton->setText(
            "GHI"
            );

        ui->jklButton->setText(
            "JKL"
            );

        ui->mnoButton->setText(
            "MNO"
            );


        ui->pqrsButton->setText(
            "PQRS"
            );

        ui->tuvButton->setText(
            "TUV"
            );

        ui->wxyzButton->setText(
            "WXYZ"
            );


        ui->shiftButton->setText(
            "Shift ↑"
            );
    }

    else
    {
        ui->punctuationButton->setText(
            ".?!"
            );


        ui->abcButton->setText(
            "abc"
            );

        ui->defButton->setText(
            "def"
            );


        ui->ghiButton->setText(
            "ghi"
            );

        ui->jklButton->setText(
            "jkl"
            );

        ui->mnoButton->setText(
            "mno"
            );


        ui->pqrsButton->setText(
            "pqrs"
            );

        ui->tuvButton->setText(
            "tuv"
            );

        ui->wxyzButton->setText(
            "wxyz"
            );


        ui->shiftButton->setText(
            "Shift ↓"
            );
    }


    ui->spaceButton->setText(
        "Space"
        );


    ui->symbolButton->setText(
        "!#1"
        );


    ui->languageButton->setText(
        QStringLiteral("한/영")
        );
}


// ============================================================
// UDP 수신 시작
// ============================================================

void MainWindow::on_bindButton_clicked()
{
    quint16 localPort =
        ui->localPortLineEdit
            ->text()
            .toUShort();


    if (localPort == 0)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("UDP Error"),
            QStringLiteral(
                "내 Port 번호를 확인해주세요."
                )
            );


        return;
    }


    udpSocket->close();


    bool success =
        udpSocket->bind(
            QHostAddress::AnyIPv4,
            localPort
            );


    if (success)
    {
        ui->bindButton->setText(
            QStringLiteral(
                "수신 중"
                )
            );


        ui->localPortLineEdit
            ->setEnabled(
                false
                );


        ui->bindButton
            ->setEnabled(
                false
                );
    }

    else
    {
        QMessageBox::warning(
            this,
            QStringLiteral("UDP Error"),
            QStringLiteral(
                "Port를 열 수 없습니다."
                )
            );
    }
}


// ============================================================
// UDP 메시지 수신
// ============================================================

void MainWindow::udpRead()
{
    while (
        udpSocket->hasPendingDatagrams()
        )
    {
        QNetworkDatagram datagram =
            udpSocket->receiveDatagram();


        QByteArray data =
            datagram.data();


        QString message =
            QString::fromUtf8(
                data
                );


        ui->chatTextEdit->append(
            QStringLiteral("상대: ")
            +
            message
            );
    }
}


// ============================================================
// Enter
//
// 천지인 입력 확정
// UDP 전송
// 채팅창 출력
// 입력 초기화
// ============================================================

void MainWindow::on_enterButton_clicked()
{
    commitCurrentCharacter();


    if (koreanMode)
    {
        commitKoreanSyllable();
    }


    if (committedText.isEmpty())
    {
        return;
    }


    QString peerIp =
        ui->peerIpLineEdit
            ->text()
            .trimmed();


    quint16 peerPort =
        ui->peerPortLineEdit
            ->text()
            .toUShort();


    QHostAddress peerAddress;


    if (!peerAddress.setAddress(
            peerIp
            ))
    {
        QMessageBox::warning(
            this,
            QStringLiteral("UDP Error"),
            QStringLiteral(
                "상대 IP 주소를 확인해주세요."
                )
            );


        return;
    }


    if (peerPort == 0)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("UDP Error"),
            QStringLiteral(
                "상대 Port 번호를 확인해주세요."
                )
            );


        return;
    }


    QByteArray data =
        committedText.toUtf8();


    qint64 result =
        udpSocket->writeDatagram(
            data,
            peerAddress,
            peerPort
            );


    if (result == -1)
    {
        QMessageBox::warning(
            this,
            QStringLiteral("UDP Error"),
            QStringLiteral(
                "메시지 전송에 실패했습니다."
                )
            );


        return;
    }


    ui->chatTextEdit->append(
        QStringLiteral("나: ")
        +
        committedText
        );


    // ========================================================
    // 입력 초기화
    // ========================================================

    committedText.clear();


    currentKey.clear();


    currentIndex =
        -1;


    inputTimer->stop();


    cho =
        -1;


    jung =
        -1;


    jong =
        0;


    vowelPattern.clear();


    updateDisplay();
}
