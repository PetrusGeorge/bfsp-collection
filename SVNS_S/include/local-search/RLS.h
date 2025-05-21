#ifndef RLS_H
#define RLS_H

#include "Instance.h"
#include "Solution.h"

bool rls(Solution &s, const std::vector<size_t> &ref, Instance &instance);

#endif
