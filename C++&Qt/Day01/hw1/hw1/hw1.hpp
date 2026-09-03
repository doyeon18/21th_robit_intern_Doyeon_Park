#ifndef HW1_HPP
#define HW1_HPP

class NumberList
{
private:
    // 필요한 멤버변수 직접 작성
    int size;
    int* arr;

    int max;
    int min;
    int sum;
    double avg;
    
    int i;
    
public:
    // 생성자
    NumberList();
    // 소멸자
    ~NumberList();
    // input
    void input();
    // calculate
    void calculate();
    // print
    void print();
};

#endif