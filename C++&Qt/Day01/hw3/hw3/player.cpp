#include <iostream>
#include "player.hpp"

using namespace std;

Player::Player()
{
    HP = 50;
    MP = 10;
    x = 0;
    y = 0;
}

Player::Player(int x, int y)
{
    HP = 50;
    MP = 10;

    this->x = x;
    this->y = y;
    // 멤버 = 매개 
}

void Player::X_move(int move)
{
    x = x + move;
}

void Player::Y_move(int move)
{
    y = y + move;
}

void Player::Show_Status()
{
    cout << "HP : " << HP << endl;
    cout << "MP : " << MP << endl;
    cout << "Position : (" << x << ", " << y << ")" << endl;
}

void Player::Attack(Monster &target)
{
    MP = MP - 1;

    if (x == target.x && y == target.y)
    {
        cout << "공격 성공!" << endl;
        cout << "남은 체력:" << target.Be_Attacked() << endl;
    }
    else
    {
        cout << "공격 실패!" << endl;
    }
}