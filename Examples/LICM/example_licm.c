int global = 5;

int bar(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        int x = 2 * 3;        
        int y = x + 4;        
        sum += y + i + global; 
    }
    return sum;
}

int baz(int* arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];        
    }
    return sum;
}

int qux(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        int x = 2 + 3;        
        int y = x * x;        
        int z = y + i;        
        sum += z;
    }
    return sum;
}

void store_somewhere(int*x)
{
    (void) x;
}

int quux(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        int x = 2 * 3;
        store_somewhere(&x);  
        sum += i;
    }
    return sum;
}

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    return bar(10) + baz(arr, 5) + qux(10) + quux(10);
}
