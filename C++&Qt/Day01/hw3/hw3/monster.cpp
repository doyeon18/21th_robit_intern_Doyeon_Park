#include <iostream>
#include "monster.hpp"

using namespace std;

Monster::Monster()
{
    // 기본값 초기화
    HP = 0;
    x = 0;
    y = 0;
}

Monster::Monster(int x, int y, int HP)
{
    // 전달받은 값으로 초기화
    this -> x = x;
    this -> y = y;
    this -> HP = HP;
    // 멤버변수 = 매개변수
}

int Monster::Be_Attacked()
{
    // HP 10 감소
    HP = HP -10;
    // 남은 HP 반환
    return HP;
}