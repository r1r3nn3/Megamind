/******************************************************************************
 * Project        : Mastermind
 * File           : Implementation Mastermind functions
 * Copyright      : Hugo Arends 2017
 ******************************************************************************
  Change History:

    Version 1.0 - July 2017
    > Initial revision

******************************************************************************/
#include "mastermind.h"

// ----------------------------------------------------------------------------
// Local variables
// ----------------------------------------------------------------------------
unsigned char secret_code[MM_DIGITS] = { MM_MARK_DONE };

// ----------------------------------------------------------------------------
// Function: This function gets the secret code that is stored in the global
//           variable secret_code[] and copies it into the buffer pointed to by
//           code.
// Pre     : The buffer has at least MM_DIGITS space available.
// Post    : -
// ----------------------------------------------------------------------------
void get_secret_code(unsigned char *code)
{
  for(uint8_t i=0; i < MM_DIGITS; i++)
    code[i] = secret_code[i];
}

// ----------------------------------------------------------------------------
// Function: This function sets the secret code by copying code[] into the
//           global variable secret_code[].
// Pre     : The size of the code[] is exactly MM_DIGITS.
// Post    : -
// ----------------------------------------------------------------------------
void set_secret_code(const unsigned char *code)
{
  for(uint8_t i=0; i < MM_DIGITS; i++)
    secret_code[i] = code[i];
}

// ----------------------------------------------------------------------------
// Function: This function calculates the result based on the user_code[] and
//           the secret_code[].
// Pre     : None of the digits in user_code and secret code is equal to
//           MM_MARK_DONE.
// Post    : -
// ----------------------------------------------------------------------------
mm_result_t check_secret_code(const unsigned char *user_code)
{
  mm_result_t result;

  // Clear the result
  result.correct_num_and_pos = 0;
  result.correct_num = 0;

  // Copy the codes
  unsigned char secret_code_tmp[MM_DIGITS];
  unsigned char user_code_tmp[MM_DIGITS];
  for(uint8_t i=0; i < MM_DIGITS; i++)
  {
    secret_code_tmp[i] = secret_code[i];
    user_code_tmp[i]   = user_code[i];
  }

  // Iteration 1
  // Check the digits that have the same value and the same position
  for(uint8_t i=0; i < MM_DIGITS; i++)
  {
    if(user_code[i] == secret_code_tmp[i])
    {
      // Mark the digits that have been done
      secret_code_tmp[i] = MM_MARK_DONE;
      user_code_tmp[i] = MM_MARK_DONE;

      result.correct_num_and_pos++;
    }
  }

  // Iteration 2
  // Check the digits that have the same value, but a different position
  // Loop all user digits (u)
  for(uint8_t u=0; u < MM_DIGITS; u++)
  {
    // Has this user digit not been counted in iteration 1?
    if(user_code_tmp[u] != MM_MARK_DONE)
    {
      // Loop all secret digits (s)
      for(uint8_t s=0; s < MM_DIGITS; s++)
      {
        // Has this secret digit not been counted in iteration 1?
        if(secret_code_tmp[s] != MM_MARK_DONE)
        {
          // Are the digits equal and the positions different?
          if((secret_code_tmp[s] == user_code[u]) && (s != u))
          {
            // Mark the secret digit so it will never be equal to a user digit again
            secret_code_tmp[s] = MM_MARK_DONE;

            result.correct_num++;

            // Do not check the rest of the secret digits for this user digit
            break;
          }
        }
      }
    }
  }

  return result;
}
