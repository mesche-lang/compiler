#ifndef mesche_fs_h
#define mesche_fs_h

#include "vm.h"

#include <stdbool.h>

bool mesche_fs_path_exists_p(const char *fs_path);
bool mesche_fs_path_absolute_p(const char *fs_path);
long mesche_fs_path_modified_time(const char *fs_path);
char *mesche_fs_resolve_path(const char *fs_path);
char *mesche_fs_file_directory(const char *file_path);
char *mesche_fs_file_read_all(const char *file_path);

void mesche_fs_module_init(VM *vm);

#endif
