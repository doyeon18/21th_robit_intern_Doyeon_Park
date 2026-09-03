#ifndef MONSTER_HPP
#define MONSTER_HPP

class Monster
{
public:
    int HP, x, y;

    Monster();
    Monster(int x, int y, int HP);

    int Be_Attacked();
};

#endif