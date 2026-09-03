#ifndef HW2_HPP
#define HW2_HPP

struct Point
{
    // x
    int x;
    // y
    int y;
};

class PointDistance
{
private:
    // 점 개수
    int count;
    // 좌표 최소/최대 범위
    int minCoord;
    int maxCoord;
    // 동적 점 배열
    Point* points;
    // 최소 거리 / 최대 거리
    double minDistance;
    double maxDistance;
    // 최소 거리를 만든 두 점 번호
    int minIndex1;
    int minIndex2;
    // 최대 거리를 만든 두 점 번호
    int maxIndex1;
    int maxIndex2;

public:
    // 생성자
    PointDistance();
    // 소멸자
    ~PointDistance();
    // 입력
    void input();
    // 랜덤 점 생성
    void generatePoints();
    // 점 출력
    void printPoints();
    // 거리 계산
    void calculate();
    // 결과 출력
    void printResult();
};

#endif