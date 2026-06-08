int global_const = 7;

int foo(int n)
{
    int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        int x = 2 * 3;
        int y = x + 1;
        sum += y + i;
    }
    return sum;
}

int bar(int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        int x = 10 + 5;
        sum += x + i;
    }
    return sum;
}

int main() { return foo(4) + bar(10); }
