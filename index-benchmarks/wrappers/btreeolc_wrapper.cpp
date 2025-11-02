#include "btreeolc_wrapper.hpp"

using LeafNode = btreeolc::BTreeLeaf<uint64_t, uint64_t>;
using InnerNode = btreeolc::BTreeInner<uint64_t>;
static_assert(sizeof(LeafNode) == btreeolc::pageSize);
static_assert(sizeof(InnerNode) == btreeolc::pageSize);

#if defined(LOCK_MEASURE_TIME)
LOCK_DEFINE_TIMING_VAR(write_cs);
#endif

extern "C" tree_api *create_tree(const tree_options_t &opt) { return new btreeolc_wrapper(opt); }
