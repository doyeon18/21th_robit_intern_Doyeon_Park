#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHeaderView>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>
#include <QMessageBox>

#include <queue>
#include <limits>
#include <chrono>
#include <algorithm>
#include <cmath>


// ============================================================
// 생성자
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    animationTimer = new QTimer(this);

    connect(
        animationTimer,
        &QTimer::timeout,
        this,
        &MainWindow::animateSearchStep
        );
}


// ============================================================
// 소멸자
// ============================================================

MainWindow::~MainWindow()
{
    delete ui;
}


// ============================================================
// 맵 생성
// ============================================================

void MainWindow::on_createMapButton_clicked()
{
    int rows = ui->rowSpinBox->value();
    int cols = ui->colSpinBox->value();

    animationTimer->stop();

    // 맵 초기화
    mapData.assign(
        rows,
        std::vector<int>(cols, 0)
        );

    startRow = -1;
    startCol = -1;

    goalRow = -1;
    goalCol = -1;


    // A* 맵 크기
    ui->astarTable->setRowCount(rows);
    ui->astarTable->setColumnCount(cols);


    // Dijkstra 맵 크기
    ui->dijkstraTable->setRowCount(rows);
    ui->dijkstraTable->setColumnCount(cols);


    // 기존 내용 삭제
    ui->astarTable->clearContents();
    ui->dijkstraTable->clearContents();


    // 행 / 열 번호 숨기기
    ui->astarTable->horizontalHeader()->setVisible(false);
    ui->astarTable->verticalHeader()->setVisible(false);

    ui->dijkstraTable->horizontalHeader()->setVisible(false);
    ui->dijkstraTable->verticalHeader()->setVisible(false);


    // 셀 최소 크기
    ui->astarTable
        ->horizontalHeader()
        ->setMinimumSectionSize(1);

    ui->astarTable
        ->verticalHeader()
        ->setMinimumSectionSize(1);

    ui->dijkstraTable
        ->horizontalHeader()
        ->setMinimumSectionSize(1);

    ui->dijkstraTable
        ->verticalHeader()
        ->setMinimumSectionSize(1);


    // 테이블에 맞게 셀 크기 조절
    ui->astarTable
        ->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    ui->astarTable
        ->verticalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    ui->dijkstraTable
        ->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);

    ui->dijkstraTable
        ->verticalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);


    // 스크롤바 제거
    ui->astarTable->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    ui->astarTable->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    ui->dijkstraTable->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    ui->dijkstraTable->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );


    // 셀 직접 편집 금지
    ui->astarTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    ui->dijkstraTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );


    ui->astarTable->clearSelection();
    ui->dijkstraTable->clearSelection();


    // 결과 초기화
    ui->astarCostLabel->setText("-");
    ui->astarVisitedLabel->setText("-");
    ui->astarTimeLabel->setText("-");

    ui->dijkstraCostLabel->setText("-");
    ui->dijkstraVisitedLabel->setText("-");
    ui->dijkstraTimeLabel->setText("-");
}


// ============================================================
// A* 맵 클릭
// ============================================================

void MainWindow::on_astarTable_cellClicked(
    int row,
    int column
    )
{
    editCell(row, column);
}


// ============================================================
// Dijkstra 맵 클릭
// ============================================================

void MainWindow::on_dijkstraTable_cellClicked(
    int row,
    int column
    )
{
    editCell(row, column);
}


// ============================================================
// 맵 편집
// ============================================================

void MainWindow::editCell(
    int row,
    int column
    )
{
    // Map Editing 모드가 아닐 경우
    if (!ui->mapEditingRadio->isChecked())
        return;

    if (mapData.empty())
        return;


    // --------------------------------------------------------
    // 장애물
    // --------------------------------------------------------

    if (ui->obstacleRadio->isChecked())
    {
        if (row == startRow &&
            column == startCol)
        {
            startRow = -1;
            startCol = -1;
        }

        if (row == goalRow &&
            column == goalCol)
        {
            goalRow = -1;
            goalCol = -1;
        }

        mapData[row][column] = 1;
    }


    // --------------------------------------------------------
    // 시작점
    // --------------------------------------------------------

    else if (ui->startRadio->isChecked())
    {
        if (startRow != -1)
        {
            int oldRow = startRow;
            int oldCol = startCol;

            mapData[oldRow][oldCol] = 0;

            updateBothTables(
                oldRow,
                oldCol
                );
        }

        if (row == goalRow &&
            column == goalCol)
        {
            goalRow = -1;
            goalCol = -1;
        }

        startRow = row;
        startCol = column;

        mapData[row][column] = 2;
    }


    // --------------------------------------------------------
    // 도착점
    // --------------------------------------------------------

    else if (ui->goalRadio->isChecked())
    {
        if (goalRow != -1)
        {
            int oldRow = goalRow;
            int oldCol = goalCol;

            mapData[oldRow][oldCol] = 0;

            updateBothTables(
                oldRow,
                oldCol
                );
        }

        if (row == startRow &&
            column == startCol)
        {
            startRow = -1;
            startCol = -1;
        }

        goalRow = row;
        goalCol = column;

        mapData[row][column] = 3;
    }


    // --------------------------------------------------------
    // 지우기
    // --------------------------------------------------------

    else if (ui->eraseRadio->isChecked())
    {
        if (row == startRow &&
            column == startCol)
        {
            startRow = -1;
            startCol = -1;
        }

        if (row == goalRow &&
            column == goalCol)
        {
            goalRow = -1;
            goalCol = -1;
        }

        mapData[row][column] = 0;
    }


    updateBothTables(
        row,
        column
        );

    ui->astarTable->clearSelection();
    ui->dijkstraTable->clearSelection();
}


// ============================================================
// 두 맵 동시에 갱신
// ============================================================

void MainWindow::updateBothTables(
    int row,
    int column
    )
{
    updateCellAppearance(
        ui->astarTable,
        row,
        column
        );

    updateCellAppearance(
        ui->dijkstraTable,
        row,
        column
        );
}


// ============================================================
// 셀 색상 표시
// ============================================================

void MainWindow::updateCellAppearance(
    QTableWidget *table,
    int row,
    int column
    )
{
    QTableWidgetItem *item =
        table->item(
            row,
            column
            );

    if (item == nullptr)
    {
        item = new QTableWidgetItem;

        table->setItem(
            row,
            column,
            item
            );
    }

    item->setText("");


    // 빈칸
    if (mapData[row][column] == 0)
    {
        item->setBackground(
            QBrush(Qt::white)
            );
    }


    // 장애물
    else if (mapData[row][column] == 1)
    {
        item->setBackground(
            QBrush(Qt::black)
            );
    }


    // 시작점
    else if (mapData[row][column] == 2)
    {
        item->setBackground(
            QBrush(Qt::red)
            );

        item->setText("S");

        item->setTextAlignment(
            Qt::AlignCenter
            );
    }


    // 도착점
    else if (mapData[row][column] == 3)
    {
        item->setBackground(
            QBrush(Qt::green)
            );

        item->setText("G");

        item->setTextAlignment(
            Qt::AlignCenter
            );
    }
}


// ============================================================
// 탐색 결과 초기화
// ============================================================

void MainWindow::resetSearchDisplay(
    QTableWidget *table
    )
{
    if (mapData.empty())
        return;

    int rows =
        static_cast<int>(
            mapData.size()
            );

    int cols =
        static_cast<int>(
            mapData[0].size()
            );

    for (int row = 0;
         row < rows;
         row++)
    {
        for (int col = 0;
             col < cols;
             col++)
        {
            updateCellAppearance(
                table,
                row,
                col
                );
        }
    }

    table->clearSelection();
}


// ============================================================
// Apply 버튼
// ============================================================

void MainWindow::on_applyButton_clicked()
{
    if (mapData.empty())
    {
        QMessageBox::warning(
            this,
            "Warning",
            "Create Map을 먼저 눌러주세요."
            );

        return;
    }


    if (startRow == -1)
    {
        QMessageBox::warning(
            this,
            "Warning",
            "Start를 설정해주세요."
            );

        return;
    }


    if (goalRow == -1)
    {
        QMessageBox::warning(
            this,
            "Warning",
            "Goal을 설정해주세요."
            );

        return;
    }


    animationTimer->stop();


    // 기존 탐색 결과 삭제
    resetSearchDisplay(
        ui->astarTable
        );

    resetSearchDisplay(
        ui->dijkstraTable
        );


    // 결과 초기화
    ui->astarCostLabel->setText("-");
    ui->astarVisitedLabel->setText("-");
    ui->astarTimeLabel->setText("-");

    ui->dijkstraCostLabel->setText("-");
    ui->dijkstraVisitedLabel->setText("-");
    ui->dijkstraTimeLabel->setText("-");


    // 두 알고리즘 실행
    currentAStarResult =
        runAStar();

    currentDijkstraResult =
        runDijkstra();


    // 탐색 과정 표시
    startSearchAnimation();
}


// ============================================================
// Stop 버튼
// ============================================================

void MainWindow::on_stopButton_clicked()
{
    animationTimer->stop();
}


// ============================================================
// Dijkstra
// ============================================================

MainWindow::SearchResult
MainWindow::runDijkstra()
{
    SearchResult result;

    int rows =
        static_cast<int>(
            mapData.size()
            );

    int cols =
        static_cast<int>(
            mapData[0].size()
            );

    const int INF =
        std::numeric_limits<int>::max();


    // 시작점부터 각 노드까지 최소 비용
    std::vector<std::vector<int>> distance(
        rows,
        std::vector<int>(
            cols,
            INF
            )
        );


    // 경로 역추적용
    std::vector<
        std::vector<
            std::pair<int, int>
            >
        > parent(
            rows,
            std::vector<std::pair<int, int>>(
                cols,
                {-1, -1}
                )
            );


    // 방문 여부
    std::vector<std::vector<bool>> visited(
        rows,
        std::vector<bool>(
            cols,
            false
            )
        );


    // 비용, 좌표
    using Node =
        std::pair<
            int,
            std::pair<int, int>
            >;


    std::priority_queue<
        Node,
        std::vector<Node>,
        std::greater<Node>
        > pq;


    auto startTime =
        std::chrono::
        high_resolution_clock::
        now();


    distance[startRow][startCol] = 0;

    pq.push(
        {
            0,
            {
                startRow,
                startCol
            }
        }
        );


    // 상하좌우
    int dr[4] =
        {
            -1,
            1,
            0,
            0
        };

    int dc[4] =
        {
            0,
            0,
            -1,
            1
        };


    while (!pq.empty())
    {
        Node current =
            pq.top();

        pq.pop();


        int currentCost =
            current.first;

        int row =
            current.second.first;

        int col =
            current.second.second;


        if (visited[row][col])
            continue;


        visited[row][col] = true;


        result.visitedOrder.push_back(
            {
                row,
                col
            }
            );


        if (row == goalRow &&
            col == goalCol)
        {
            break;
        }


        for (int i = 0;
             i < 4;
             i++)
        {
            int nextRow =
                row + dr[i];

            int nextCol =
                col + dc[i];


            if (nextRow < 0 ||
                nextRow >= rows ||
                nextCol < 0 ||
                nextCol >= cols)
            {
                continue;
            }


            if (mapData[nextRow][nextCol] == 1)
            {
                continue;
            }


            int newCost =
                currentCost + 1;


            if (newCost <
                distance[nextRow][nextCol])
            {
                distance[nextRow][nextCol] =
                    newCost;

                parent[nextRow][nextCol] =
                    {
                        row,
                        col
                    };

                pq.push(
                    {
                        newCost,
                        {
                            nextRow,
                            nextCol
                        }
                    }
                    );
            }
        }
    }


    auto endTime =
        std::chrono::
        high_resolution_clock::
        now();


    result.timeMs =
        std::chrono::
        duration<
            double,
            std::milli
            >(
            endTime - startTime
            ).count();


    result.visitedCount =
        static_cast<int>(
            result.visitedOrder.size()
            );


    if (distance[goalRow][goalCol] == INF)
    {
        result.cost = -1;

        return result;
    }


    result.cost =
        distance[goalRow][goalCol];


    // Goal -> Start 역추적
    int row = goalRow;
    int col = goalCol;


    while (!(row == startRow &&
             col == startCol))
    {
        result.path.push_back(
            {
                row,
                col
            }
            );

        std::pair<int, int> previous =
            parent[row][col];

        row = previous.first;
        col = previous.second;
    }


    result.path.push_back(
        {
            startRow,
            startCol
        }
        );


    std::reverse(
        result.path.begin(),
        result.path.end()
        );


    return result;
}


// ============================================================
// A* 휴리스틱
//
// Manhattan Distance
// ============================================================

int MainWindow::heuristic(
    int row,
    int col
    )
{
    return
        std::abs(row - goalRow)
        +
        std::abs(col - goalCol);
}


// ============================================================
// A*
// ============================================================

MainWindow::SearchResult
MainWindow::runAStar()
{
    SearchResult result;


    int rows =
        static_cast<int>(
            mapData.size()
            );

    int cols =
        static_cast<int>(
            mapData[0].size()
            );


    const int INF =
        std::numeric_limits<int>::max();


    // ========================================================
    // g(n)
    // 시작점 -> 현재 노드 실제 비용
    // ========================================================

    std::vector<std::vector<int>> gCost(
        rows,
        std::vector<int>(
            cols,
            INF
            )
        );


    // 경로 역추적
    std::vector<
        std::vector<
            std::pair<int, int>
            >
        > parent(
            rows,
            std::vector<std::pair<int, int>>(
                cols,
                {-1, -1}
                )
            );


    // 방문 여부
    std::vector<std::vector<bool>> visited(
        rows,
        std::vector<bool>(
            cols,
            false
            )
        );


    // ========================================================
    // A*에서 사용할 Node
    //
    // f = g + h
    // h = Goal까지 예상 거리
    // ========================================================

    struct AStarNode
    {
        int f;
        int h;

        int row;
        int col;
    };


    // ========================================================
    // 우선순위 비교
    //
    // 1. f가 작은 노드 먼저
    //
    // 2. f가 같다면 h가 작은 노드 먼저
    //
    // 즉 Goal에 더 가까운 노드를 우선
    // ========================================================

    struct CompareNode
    {
        bool operator()(
            const AStarNode &a,
            const AStarNode &b
            ) const
        {
            if (a.f != b.f)
            {
                return a.f > b.f;
            }

            return a.h > b.h;
        }
    };


    std::priority_queue<
        AStarNode,
        std::vector<AStarNode>,
        CompareNode
        > pq;


    auto startTime =
        std::chrono::
        high_resolution_clock::
        now();


    // 시작점 g = 0
    gCost[startRow][startCol] = 0;


    // 시작점 h
    int startH =
        heuristic(
            startRow,
            startCol
            );


    // 시작점 f = g + h
    int startF =
        0 + startH;


    pq.push(
        {
            startF,
            startH,
            startRow,
            startCol
        }
        );


    // 상하좌우
    int dr[4] =
        {
            -1,
            1,
            0,
            0
        };

    int dc[4] =
        {
            0,
            0,
            -1,
            1
        };


    while (!pq.empty())
    {
        AStarNode current =
            pq.top();

        pq.pop();


        int row =
            current.row;

        int col =
            current.col;


        // 이미 방문 완료
        if (visited[row][col])
            continue;


        visited[row][col] = true;


        result.visitedOrder.push_back(
            {
                row,
                col
            }
            );


        // Goal 도착
        if (row == goalRow &&
            col == goalCol)
        {
            break;
        }


        // ====================================================
        // 상하좌우
        // ====================================================

        for (int i = 0;
             i < 4;
             i++)
        {
            int nextRow =
                row + dr[i];

            int nextCol =
                col + dc[i];


            // 맵 바깥
            if (nextRow < 0 ||
                nextRow >= rows ||
                nextCol < 0 ||
                nextCol >= cols)
            {
                continue;
            }


            // 장애물
            if (mapData[nextRow][nextCol] == 1)
            {
                continue;
            }


            // 현재 노드까지 비용 + 1
            int newG =
                gCost[row][col] + 1;


            // 더 짧은 경로 발견
            if (newG <
                gCost[nextRow][nextCol])
            {
                gCost[nextRow][nextCol] =
                    newG;


                parent[nextRow][nextCol] =
                    {
                        row,
                        col
                    };


                // Goal까지 예상 거리
                int h =
                    heuristic(
                        nextRow,
                        nextCol
                        );


                // A* 우선순위
                int f =
                    newG + h;


                pq.push(
                    {
                        f,
                        h,
                        nextRow,
                        nextCol
                    }
                    );
            }
        }
    }


    auto endTime =
        std::chrono::
        high_resolution_clock::
        now();


    result.timeMs =
        std::chrono::
        duration<
            double,
            std::milli
            >(
            endTime - startTime
            ).count();


    result.visitedCount =
        static_cast<int>(
            result.visitedOrder.size()
            );


    // 경로를 못 찾은 경우
    if (gCost[goalRow][goalCol] == INF)
    {
        result.cost = -1;

        return result;
    }


    result.cost =
        gCost[goalRow][goalCol];


    // Goal -> Start 역추적
    int row = goalRow;
    int col = goalCol;


    while (!(row == startRow &&
             col == startCol))
    {
        result.path.push_back(
            {
                row,
                col
            }
            );


        std::pair<int, int> previous =
            parent[row][col];


        row = previous.first;
        col = previous.second;
    }


    result.path.push_back(
        {
            startRow,
            startCol
        }
        );


    std::reverse(
        result.path.begin(),
        result.path.end()
        );


    return result;
}


// ============================================================
// 탐색 애니메이션 시작
// ============================================================

void MainWindow::startSearchAnimation()
{
    astarVisitIndex = 0;
    dijkstraVisitIndex = 0;

    astarPathIndex = 0;
    dijkstraPathIndex = 0;

    showingPaths = false;


    int delay =
        ui->animationSpinBox->value();


    animationTimer->start(
        delay
        );
}


// ============================================================
// 탐색한 셀 색칠
// ============================================================

void MainWindow::colorVisitedCell(
    QTableWidget *table,
    int row,
    int col
    )
{
    // 시작 / 도착점 유지
    if ((row == startRow &&
         col == startCol) ||
        (row == goalRow &&
         col == goalCol))
    {
        return;
    }


    QTableWidgetItem *item =
        table->item(
            row,
            col
            );


    if (item == nullptr)
    {
        item =
            new QTableWidgetItem;

        table->setItem(
            row,
            col,
            item
            );
    }


    item->setBackground(
        QBrush(
            QColor(
                173,
                216,
                230
                )
            )
        );
}


// ============================================================
// 최단 경로 색칠
// ============================================================

void MainWindow::colorPathCell(
    QTableWidget *table,
    int row,
    int col
    )
{
    // 시작 / 도착점 유지
    if ((row == startRow &&
         col == startCol) ||
        (row == goalRow &&
         col == goalCol))
    {
        return;
    }


    QTableWidgetItem *item =
        table->item(
            row,
            col
            );


    if (item == nullptr)
    {
        item =
            new QTableWidgetItem;

        table->setItem(
            row,
            col,
            item
            );
    }


    item->setBackground(
        QBrush(Qt::yellow)
        );
}


// ============================================================
// 탐색 애니메이션
// ============================================================

void MainWindow::animateSearchStep()
{
    // ========================================================
    // 1단계 : 방문한 노드 표시
    // ========================================================

    if (!showingPaths)
    {
        bool astarDone =
            astarVisitIndex >=
            static_cast<int>(
                currentAStarResult
                    .visitedOrder
                    .size()
                );


        bool dijkstraDone =
            dijkstraVisitIndex >=
            static_cast<int>(
                currentDijkstraResult
                    .visitedOrder
                    .size()
                );


        // A*
        if (!astarDone)
        {
            auto cell =
                currentAStarResult
                    .visitedOrder[
                        astarVisitIndex
            ];

            astarVisitIndex++;

            colorVisitedCell(
                ui->astarTable,
                cell.first,
                cell.second
                );
        }


        // Dijkstra
        if (!dijkstraDone)
        {
            auto cell =
                currentDijkstraResult
                    .visitedOrder[
                        dijkstraVisitIndex
            ];

            dijkstraVisitIndex++;

            colorVisitedCell(
                ui->dijkstraTable,
                cell.first,
                cell.second
                );
        }


        astarDone =
            astarVisitIndex >=
            static_cast<int>(
                currentAStarResult
                    .visitedOrder
                    .size()
                );


        dijkstraDone =
            dijkstraVisitIndex >=
            static_cast<int>(
                currentDijkstraResult
                    .visitedOrder
                    .size()
                );


        if (astarDone &&
            dijkstraDone)
        {
            showingPaths = true;
        }


        return;
    }


    // ========================================================
    // 2단계 : 최단 경로 표시
    // ========================================================

    bool astarPathDone =
        astarPathIndex >=
        static_cast<int>(
            currentAStarResult
                .path
                .size()
            );


    bool dijkstraPathDone =
        dijkstraPathIndex >=
        static_cast<int>(
            currentDijkstraResult
                .path
                .size()
            );


    // A* 경로
    if (!astarPathDone)
    {
        auto cell =
            currentAStarResult
                .path[
                    astarPathIndex
        ];

        astarPathIndex++;

        colorPathCell(
            ui->astarTable,
            cell.first,
            cell.second
            );
    }


    // Dijkstra 경로
    if (!dijkstraPathDone)
    {
        auto cell =
            currentDijkstraResult
                .path[
                    dijkstraPathIndex
        ];

        dijkstraPathIndex++;

        colorPathCell(
            ui->dijkstraTable,
            cell.first,
            cell.second
            );
    }


    astarPathDone =
        astarPathIndex >=
        static_cast<int>(
            currentAStarResult
                .path
                .size()
            );


    dijkstraPathDone =
        dijkstraPathIndex >=
        static_cast<int>(
            currentDijkstraResult
                .path
                .size()
            );


    if (astarPathDone &&
        dijkstraPathDone)
    {
        finishSearchAnimation();
    }
}


// ============================================================
// 탐색 종료
// ============================================================

void MainWindow::finishSearchAnimation()
{
    animationTimer->stop();


    // ========================================================
    // A*
    // ========================================================

    if (currentAStarResult.cost == -1)
    {
        ui->astarCostLabel->setText(
            "No Path"
            );
    }
    else
    {
        ui->astarCostLabel->setText(
            QString::number(
                currentAStarResult.cost
                )
            );
    }


    ui->astarVisitedLabel->setText(
        QString::number(
            currentAStarResult.visitedCount
            )
        );


    ui->astarTimeLabel->setText(
        QString::number(
            currentAStarResult.timeMs,
            'f',
            3
            )
        +
        " ms"
        );


    // ========================================================
    // Dijkstra
    // ========================================================

    if (currentDijkstraResult.cost == -1)
    {
        ui->dijkstraCostLabel->setText(
            "No Path"
            );
    }
    else
    {
        ui->dijkstraCostLabel->setText(
            QString::number(
                currentDijkstraResult.cost
                )
            );
    }


    ui->dijkstraVisitedLabel->setText(
        QString::number(
            currentDijkstraResult.visitedCount
            )
        );


    ui->dijkstraTimeLabel->setText(
        QString::number(
            currentDijkstraResult.timeMs,
            'f',
            3
            )
        +
        " ms"
        );


    // S / G 다시 표시
    updateCellAppearance(
        ui->astarTable,
        startRow,
        startCol
        );

    updateCellAppearance(
        ui->astarTable,
        goalRow,
        goalCol
        );

    updateCellAppearance(
        ui->dijkstraTable,
        startRow,
        startCol
        );

    updateCellAppearance(
        ui->dijkstraTable,
        goalRow,
        goalCol
        );


    ui->astarTable->clearSelection();
    ui->dijkstraTable->clearSelection();
}
