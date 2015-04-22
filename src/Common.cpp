#include "Common.h"

#include <cmath>

using namespace std;
using namespace sge;

bool sge::weakEq(sge::ftype a, sge::ftype b)
{
    if (abs(a - b) < FTYPE_WEAK_EPS)
        // if it is absolutely close, return true
        return true;
    
    if (abs(b) > FTYPE_WEAK_EPS)
        // if it's less than eps and not absolutely close, it can't be relatively close
        return false;
    
    // check for relative proximity
    return abs(a / b - 1.0) < FTYPE_WEAK_EPS;
}