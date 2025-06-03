#ifndef RLS_H
#define RLS_H

#include <vector>

#include "Instance.h"
#include "Solution.h"

bool rls(Solution &s, const std::vector<size_t> &ref, Instance &instance);
bool rls_grabowski(Solution &s, const std::vector<size_t> &ref, Instance &instance);

#endif