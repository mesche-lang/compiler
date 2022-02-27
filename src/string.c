#include "string.h"
#include <stdlib.h>

char *mesche_cstring_join(const char *left, size_t left_length, const char *right,
                          size_t right_length, const char *separator) {
  size_t separator_len = 0;
  size_t new_length = left_length + right_length + 1;
  if (separator != NULL) {
    separator_len = strlen(separator);
    new_length += separator_len;
  }

  // Allocate a temporary buffer into which we'll copy the string parts
  char *string_buffer = malloc(sizeof(char) * new_length);
  char *copy_ptr = string_buffer;

  // Copy the requisite string segments into the new buffer
  memcpy(copy_ptr, left, left_length);
  copy_ptr += left_length;
  if (separator) {
    memcpy(copy_ptr, separator, separator_len);
    copy_ptr += separator_len;
  }
  memcpy(copy_ptr, right, right_length);

  // Add the null char
  string_buffer[new_length - 1] = '\0';

  // Caller is responsible for freeing
  return string_buffer;
}

ObjectString *mesche_string_join(VM *vm, ObjectString *left, ObjectString *right,
                                 const char *separator) {
  // Join the two cstrings
  char *joined_string =
      mesche_cstring_join(left->chars, left->length, right->chars, right->length, separator);

  // Allocate the new string, free the temporary buffer, and return
  // TODO: Try to get new length from join function, don't use strlen
  ObjectString *new_string = mesche_object_make_string(vm, joined_string, strlen(joined_string));
  free(joined_string);
  return new_string;
}
