#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QTimer>

#include <vector>
#include <utility>

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
    void on_createMapButton_clicked();

    void on_astarTable_cellClicked(int row, int column);
    void on_dijkstraTable_cellClicked(int row, int column);

    void on_applyButton_clicked();
    void on_stopButton_clicked();

    void animateSearchStep();

private:
    Ui::MainWindow *ui;

    // 0 : 빈칸
    // 1 : 장애물
    // 2 : 시작점
    // 3 : 도착점
    std::vector<std::vector<int>> mapData;

    int startRow = -1;
    int startCol = -1;

    int goalRow = -1;
    int goalCol = -1;


    // 탐색 결과
    struct SearchResult
    {
        int cost = -1;
        int visitedCount = 0;
        double timeMs = 0.0;

        std::vector<std::pair<int, int>> visitedOrder;
        std::vector<std::pair<int, int>> path;
    };


    // 맵 편집
    void editCell(int row, int column);

    void updateCellAppearance(
        QTableWidget *table,
        int row,
        int column
        );

    void updateBothTables(
        int row,
        int column
        );

    void resetSearchDisplay(
        QTableWidget *table
        );


    // 알고리즘
    SearchResult runDijkstra();

    SearchResult runAStar();

    int heuristic(
        int row,
        int col
        );


    // 애니메이션
    QTimer *animationTimer;

    SearchResult currentDijkstraResult;
    SearchResult currentAStarResult;

    int dijkstraVisitIndex = 0;
    int astarVisitIndex = 0;

    int dijkstraPathIndex = 0;
    int astarPathIndex = 0;

    bool showingPaths = false;

    void startSearchAnimation();
    void finishSearchAnimation();

    void colorVisitedCell(
        QTableWidget *table,
        int row,
        int col
        );

    void colorPathCell(
        QTableWidget *table,
        int row,
        int col
        );
};

#endif
