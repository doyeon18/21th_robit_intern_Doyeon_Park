#include <iostream>
#include "player.hpp"
#include "monster.hpp"

using namespace std;

int main()
{
    Player player(0, 0);
    Monster monster(5, 4, 50);

    char command;

    while (true)
    {
        cout << "Type Command(A/U/D/R/L/S) : ";
        cin >> command;

        if (command == 'U')
        {
            player.Y_move(1);
            cout << "Y Position 1 moved!" << endl;
        }

        else if (command == 'D')
        {
            player.Y_move(-1);
            cout << "Y Position -1 moved!" << endl;
        }

        else if (command == 'R')
        {
            player.X_move(1);
            cout << "X Position 1 moved!" << endl;
        }

        else if (command == 'L')
        {
            player.X_move(-1);
            cout << "X Position -1 moved!" << endl;
        }

        else if (command == 'S')
        {
            player.Show_Status();
        }

        else if (command == 'A')
        {
            if (player.MP <= 0)
            {
                cout << "MP 부족!" << endl;
                break;
            }

            player.Attack(monster);

            if (monster.HP <= 0)
            {
                cout << "Monster Die!!" << endl;
                break;
            }
        }

        else
        {
            cout << "Wrong Command!" << endl;
        }

        cout << endl;
    }

    return 0;
}