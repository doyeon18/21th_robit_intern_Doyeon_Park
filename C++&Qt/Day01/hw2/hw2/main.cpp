#include <cstdlib>
#include <ctime>
#include "hw2.hpp"

int main(void)
{
    srand(time(nullptr));

    PointDistance pointdistance;
    
    pointdistance.input();

    pointdistance.generatePoints();

    pointdistance.printPoints();

    pointdistance.calculate();

    pointdistance.printResult();

    return 0;
}