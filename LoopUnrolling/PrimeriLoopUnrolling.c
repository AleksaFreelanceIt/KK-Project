#include<stdio.h>

#define MAX 100

int main() {

int x, n;
int a[MAX];

scanf("%d", &n);

// za partial unrolling - 1 BasicBlock - samo n=3 za full
for(int i = 0; i < n; i++) {
    a[i%2] = x * 3;
}

// za partial unrolling - vise BasicBlocks - samo n=3 za full
for(int i = 0; i < n; i++) {
    a[i] = i*2;

    if(a[i] % 3 == 0) {
        x = x + a[i];
    } else {
        x = x-1;
    }
}

// full
for(int i = 0; i < 5; i++) {
    if(x > 10) {
        x++;
    } else if(x > 0) {
        x = x * 2;
    } else {
        a[i] = a[i] % 2;
    }
}

return 0;
}
