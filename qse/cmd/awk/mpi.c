

#define ENABLE_MPI

#define qse_awk_openstd         qse_awk_openmpi
#define qse_awk_openstdwithmmgr qse_awk_openmpiwithmmgr
#define qse_awk_getxtnstd       qse_awk_getxtnmpi
#define qse_awk_parsestd        qse_awk_parsempi
#define qse_awk_rtx_openstd     qse_awk_rtx_openmpi
#define qse_awk_rtx_getxtnstd   qse_awk_rtx_getxtnmpi
#define qse_awk_rtx_getcmgrstd  qse_awk_rtx_getcmgrmpi

#include "awk.c"

