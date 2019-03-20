#ifndef __TTPOD_TT_DISPLAY_H_
#define __TTPOD_TT_DISPLAY_H_

#include <stdint.h>

/**
 * Initialize a transformation matrix describing a pure rotation by the
 * specified angle (in degrees).
 *
 * @param matrix an allocated transformation matrix (will be fully overwritten
 *               by this function)
 * @param angle rotation angle in degrees.
 */
void ttv_display_rotation_set(int32_t matrix[9], double angle);

/**
 * Flip the input matrix horizontally and/or vertically.
 *
 * @param matrix an allocated transformation matrix
 * @param hflip whether the matrix should be flipped horizontally
 * @param vflip whether the matrix should be flipped vertically
 */
void ttv_display_matrix_flip(int32_t matrix[9], int hflip, int vflip);

#endif /* __TTPOD_TT_DISPLAY_H_ */
