#ifndef LBJ_TRAIN_TYPE_H
#define LBJ_TRAIN_TYPE_H
#include <stddef.h>

/* out is encoded in GB2312, matching the stock LOSEHU small-font renderer. */
void LBJ_IdentifyTrainType(const char *train_no, char *out, size_t out_size);

#endif
