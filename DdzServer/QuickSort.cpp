//
// Quick Sort Algorithm
//
#include "stdafx.h"
#include "QuickSort.h"

int qs_partition(int a[], int low, int high)
{
    int i, j;
    int compare;

    i = low;
    j = high;
    compare = a[low];

    while (i < j) { 
        while ((i < j) && (compare <= a[j])) {
            j--;
        }

        if (i < j) {
            a[i] = a[j];
            i++;
        }

        while ((i < j) && (compare > a[i])) {
            i++;
        }

        if (i < j) {
            a[j] = a[i];
            j--;
        }
    }

    a[i] = compare;

    return i;
}

void quick_sort(int a[], int low, int high)
{
    int pos;

    if (low < high) {
        pos = qs_partition(a, low, high);
        quick_sort(a, low, pos - 1);
        quick_sort(a, pos + 1, high);
    }
}
