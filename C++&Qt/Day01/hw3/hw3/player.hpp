#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "monster.hpp"

class Player
{
public:
    int HP, MP, x, y;

    Player();
    Player(int x, int y);

    void Attack(Monster &target);
    void Show_Status();
    void X_move(int move);
    void Y_move(int move);
};

#endif