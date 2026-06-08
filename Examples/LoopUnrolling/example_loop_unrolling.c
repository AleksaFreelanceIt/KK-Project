
int foo(int* a)
{
    int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        sum += i * 2;
    }
    return sum;
}

int bar(int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += i * 3;
    }
    return sum;
}

int baz(int* a)
{
    int sum = 0;
    for (int i = 0; i < 3; i++)
    {
        if (a[i] > 0)
            sum += a[i];
        else
            sum -= a[i];
    }
    return sum;
}

int qux(int* a, int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        if (a[i] > 0)
            sum += a[i];
        else
            sum -= a[i];
    }
    return sum;
}

int quux(int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += i;
        i++;
    }
    return sum;
}

int main()
{
    int a[] = {1, -2, 3, -4};
    return foo(a) + bar(10) + baz(a) + qux(a, 4) + quux(10);
}
