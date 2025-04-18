// Author: Serhij Čepil (sipxi)
// FIT VUT Student
// https://github.com/sipxi
// Date: 18/05/2025
// Time spent: 2h
#include <stdio.h>
#include "main.h"

// --- Argument count ---
#define EXPECTED_ARGS 6

// --- Limits ---
#define MAX_NUM_TRUCKS         10000
#define MAX_NUM_CARS           10000

// --- Capacity constraints ---
#define MIN_CAPACITY_TRAFFIC   3
#define MAX_CAPACITY_TRAFFIC   100

// --- Timing constraints (microseconds) ---
#define MIN_CAR_ARRIVAL_US     0
#define MAX_CAR_ARRIVAL_US     10000
#define MIN_TRUCK_ARRIVAL_US   0
#define MAX_TRUCK_ARRIVAL_US   1000


typedef enum {
    EX_SUCCESS,
    EX_ERROR_ARGS,
} ReturnCode;


int parse_args(int argc, char const *argv[])
{
    if (argc != EXPECTED_ARGS)
    {
        // send to stderr
        fprintf(stderr, "[ERROR:%d] Expected %d arguments, got %d\n",
            EX_ERROR_ARGS, EXPECTED_ARGS, argc);
        return EX_ERROR_ARGS;
    }
    
    for (int i = 1; i < argc; i++)
    {
        switch (i)
        {
        case 1:
            atoi(argv[i]);
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            break;
        }
    }
    return EX_SUCCESS;
}


int main(int argc, char const *argv[])
{   
    printf("Number of arguments: %d \n", argc);
    printf("Argc value: %d \n", argc);
    
    for (int i = 0; i < argc; i++)
    {
        printf("\n %d. %s ",i, argv[i]);
    }

    printf("\n");

    parse_args(argc, argv);

    return EX_SUCCESS;

}
