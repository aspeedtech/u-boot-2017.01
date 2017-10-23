#if defined(CONFIG_IRMP) || defined(CONFIG_REMOTEFX) || defined(CONFIG_PCEXT)
#include "ast-bmc-scu.h"
#elif defined(CONFIG_RT360_CAM)
#include "ast-cam-scu.h"
#else
#err "no define for ast-scu.h"
#endif
