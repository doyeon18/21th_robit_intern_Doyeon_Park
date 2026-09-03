#include <iostream>
#include "hw1.hpp"

using namespace std;

NumberList::NumberList()
{
    size = 0;
    arr = nullptr;
    max = 0;
    min = 0;
    sum = 0;
    avg = 0;
    i = 0;
}

NumberList::~NumberList()
{
    delete[] arr;
}

void NumberList::input()
{
    cout << "몇 개의 원소를 할당하겠습니까? : ";
    cin >> size;

    arr = new int[size];

    for (i = 0; i < size; i++)
    {
        cout << "정수형 데이터 입력 : ";
        cin >> arr[i];
    }
}

void NumberList::calculate()
{
    max = arr[0];
    min = arr[0];
    sum = 0;

    for (i = 0; i < size; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }

        if (arr[i] < min)
        {
            min = arr[i];
        }

        sum = sum + arr[i];
    }

    avg = static_cast<double>(sum) / size;
}

void NumberList::print()
{
    cout << "최대값 : " << max << endl;
    cout << "최솟값 : " << min << endl;
    cout << "전체합 : " << sum << endl;
    cout << "평 균 : " << avg << endl;
}