#include <iostream>
#include <cstdlib>
#include <cmath>
#include "hw2.hpp"

using namespace std;

PointDistance::PointDistance()
{
    count = 0;
    minCoord = 0;
    maxCoord = 0;
    points = nullptr;
    minDistance = 0;
    maxDistance = 0;
    minIndex1 = 0;
    minIndex2 = 0;
    maxIndex1 = 0;
    maxIndex2 = 0;
}

PointDistance::~PointDistance()
{
    delete[] points;
}

void PointDistance::input()
{
    cout << "Please define the number of points: ";
    cin >> count;

    cout << "Please define minimum of coor. value: ";
    cin >> minCoord;
    cout << "Please define maximum of coor. value: ";
    cin >> maxCoord;

    points = new Point[count];
}

void PointDistance::generatePoints()
{
    for (int i = 0; i < count; i++)
    {
        points[i].x = rand() % (maxCoord - minCoord + 1) + minCoord;
        points[i].y = rand() % (maxCoord - minCoord + 1) + minCoord;
    }
}

void PointDistance::printPoints()
{
    for (int i = 0; i < count; i++) {
        cout << "Point " << i + 1 << " : ("
        << points[i].x << ", "
        << points[i].y << ")" << endl;
    }
}

void PointDistance::calculate()
{
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            double distance = sqrt(
                (points[i].x - points[j].x) * (points[i].x - points[j].x)
                +
                (points[i].y - points[j].y) * (points[i].y - points[j].y)
            );
            if (i == 0 && j == 1) {
                minDistance = distance;
                maxDistance = distance;

                minIndex1 = i;
                minIndex2 = j;

                maxIndex1 = i;
                maxIndex2 = j;
            }
            else
            {
                if (distance < minDistance) {
                    minDistance = distance;
                    minIndex1 = i;
                    minIndex2 = j;
                }

                if (distance > maxDistance) {
                    maxDistance = distance;
                    maxIndex1 = i;
                    maxIndex2 = j;
                }
            }
        }
    }
}

void PointDistance::printResult()
{
    cout << "-----result-----" << endl;

    cout << "Minimum distance : ";
    cout << minDistance << endl;
    cout << "Point " << minIndex1 + 1 << ": " << "(" << points[minIndex1].x << ", " << points[minIndex1].y << ")" << endl;
    cout << "Point " << minIndex2 + 1 << ": " << "(" << points[minIndex2].x << ", " << points[minIndex2].y << ")" << endl;

    cout << "Maximum distance : ";
    cout << maxDistance << endl;
    cout << "Point " << maxIndex1 + 1 << ": " << "(" << points[maxIndex1].x << ", " << points[maxIndex1].y << ")" << endl;
    cout << "Point " << maxIndex2 + 1 << ": " << "(" << points[maxIndex2].x << ", " << points[maxIndex2].y << ")" << endl;
}
    