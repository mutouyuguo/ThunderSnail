#include "common_base_struct/common_base_struct.h"

typedef struct {
    int tupleIDCount;
    int maxLinkAddrCount;
    int hashAddrCount;
    uint8_t buffer[]; // [[Tuple IDs], [MaxLink Addrs], [Hash Addrs]]
} NewLinkT;

TupleIdT* NewLinkGetTupleIDs(NewLinkT* newLink);
MaxLinkAddrT* NewLinkGetMaxLinkAddrs(NewLinkT* newLink);
HashAddrT* NewLinkGetHashAddrs(NewLinkT* newLink);
int NewLinkGetSize(int tupleIdCount, int maxLinkAddrCount, int hashAddrCount);
void NewLinkPrint(NewLinkT* newLink);
void NewLinkTest();