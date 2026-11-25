#include <stdio.h>
#define PAGE_SIZE 1024
#define PAGES 8
/* Page table of a process: frame number, or -1 if the page is not in memory. */
int page_table[PAGES] = {5, 2, -1, 7, 0, -1, 3, -1};
int main(void)
{
unsigned logical[] = {0, 1500, 3100, 6200, 9000, 2049};
for (int i = 0; i < 6; i++) {
unsigned a = logical[i];
    unsigned p = a / PAGE_SIZE, d = a % PAGE_SIZE;           /* TODO 1 completed */
    printf("%u -> page %u offset %u : ", a, p, d);
    if (p >= PAGES)                printf("invalid address\n");
    else if (page_table[p] == -1)  printf("page fault\n");
    else                           printf("physical %u\n", page_table[p] * PAGE_SIZE + d);
}
return 0;
}
