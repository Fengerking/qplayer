#include <stdint.h>
#include <string.h>
#include <math.h>

#include "ttDisplay.h"
#include "ttMathematics.h"

// fixed point to double
#define CONV_FP(x) ((double) (x)) / (1 << 16)

// double to fixed point
#define CONV_DB(x) (int32_t) ((x) * (1 << 16))

void ttv_display_rotation_set(int32_t matrix[9], double angle)
{
    double radians = angle * M_PI / 180.0f;
    double c = cos(radians);
    double s = sin(radians);

    memset(matrix, 0, 9 * sizeof(int32_t));

    matrix[0] = CONV_DB(c);
    matrix[1] = CONV_DB(-s);
    matrix[3] = CONV_DB(s);
    matrix[4] = CONV_DB(c);
    matrix[8] = 1 << 30;
}

void ttv_display_matrix_flip(int32_t matrix[9], int hflip, int vflip)
{
    int i;
    const int flip[] = { 1 - 2 * (!!hflip), 1 - 2 * (!!vflip), 1 };

    if (hflip || vflip)
        for (i = 0; i < 9; i++)
            matrix[i] *= flip[i % 3];
}
