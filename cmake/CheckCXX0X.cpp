// Determine C++0x support by checking the r-value reference availability

#include <utility>

struct Test
{
    Test& operator=(Test&&)
    {
        return *this;
    }
};

int main()
{
    Test t1, t2;
    t1 = std::move(t2);
}
