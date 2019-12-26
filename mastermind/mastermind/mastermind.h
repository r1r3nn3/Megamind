/******************************************************************************
 * Project        : Mastermind
 * File           : Header Mastermind functions
 * Copyright      : Hugo Arends 2017
 ******************************************************************************
  Change History:

    Version 1.0 - July 2017
    > Initial revision

******************************************************************************/
#ifndef _MASTERMIND_H_
#define _MASTERMIND_H_

#include <stdint.h>

// ----------------------------------------------------------------------------
// Defines
// ----------------------------------------------------------------------------
#define MM_DIGITS    (  4)
#define MM_MARK_DONE (255)

typedef struct mm_result_t
{
  uint8_t correct_num_and_pos;
  uint8_t correct_num;
} mm_result_t;

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------
void        get_secret_code(unsigned char *);

void        set_secret_code(const unsigned char *);

mm_result_t check_secret_code(const unsigned char *);

#endif /* _MASTERMIND_H_ */
