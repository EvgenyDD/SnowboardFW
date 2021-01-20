#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "fatfs.h"

int file_op_log_enable(void);
int file_op_log(const void *buf, UINT len);
void file_op_log_disable(void);

void file_op_init(void);

void file_op_test(void);

#endif // FILE_OPERATIONS_H