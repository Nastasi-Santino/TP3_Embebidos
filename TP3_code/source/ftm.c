#include "pdb.h"
#include "hardware.h"

static FTM_Type * const ftms[] = FTM_BASE_PTRS;
static IRQn_Type pdb_IRQn[] = FTM_IRQS;
