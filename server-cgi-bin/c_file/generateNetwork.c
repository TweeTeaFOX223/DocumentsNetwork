#include <stdio.h>
#include <string.h>
#include "se.h"
#include "mds.h"
#include "ce.h"
#include "kk.h"

void generateNetwork(
    const char **netArgs,
    const char *selectedNetwork
){
    if      (strcmp(selectedNetwork, "SE")  == 0) se(netArgs);
    else if (strcmp(selectedNetwork, "MDS") == 0) mds(netArgs);
    else if (strcmp(selectedNetwork, "CE")  == 0) ce(netArgs);
    else if (strcmp(selectedNetwork, "KK")  == 0) kk(netArgs);
    else                                          kk(netArgs);
}
