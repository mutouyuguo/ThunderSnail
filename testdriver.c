#include <stdio.h>
#include "common_base_struct/common_base_struct.h"
#include "newlink/disjoint_set.h"
#include "newlink/hash_table_for_newlink.h"
#include "newlink/variable_length_struct_buffer.h"

int main() {
    // assert(GetMaxLinkSize(5, 5) == 128);
    // assert(DisjointSetTest());
    // assert(HashTableForNewLinkTest());
    assert(VariableLengthStructBufferTest());
    return 0;
}