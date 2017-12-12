#pragma once
#include <unordered_map>
#include "npylm/hashmap/hashmap.h"
#include "npylm/hashmap/flat_hashmap.h"

#ifdef __NO_INLINE__
#define __DEBUG__ 1
#endif

template<class K, class V>
using hashmap = ska::flat_hash_map<K, V, ska::power_of_two_std_hash<K>>;	// これが最速
// using hashmap = ska::flat_hash_map<K, V>;
// using hashmap = emilib::HashMap<K, V>;
// using hashmap = std::unordered_map<K, V>;

using id = size_t;

#define HPYLM_INITIAL_D 	0.5
#define HPYLM_INITIAL_THETA 2.0
#define HPYLM_BETA_A 	1.0
#define HPYLM_BETA_B 	1.0
#define HPYLM_GAMMA_ALPHA 1.0
#define HPYLM_GAMMA_BETA  1.0

#define VPYLM_BETA_STOP  4
#define VPYLM_BETA_PASS  1
#define VPYLM_EPS 1e-12

#define SPECIAL_CHARACTER_UNK 0
#define SPECIAL_CHARACTER_BEGIN 1
#define SPECIAL_CHARACTER_END 2