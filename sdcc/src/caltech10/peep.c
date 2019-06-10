#include "peep.h"

// functions currently do nothing, placeholders
int getSize_ (lineNode * line)
{
    return 0;
}

bitVect getRegsRead (lineNode * line)
{
    bitVect v;
    return v;
}

bitVect getRegsWritten (lineNode * line)
{
    bitVect v;
    return v;
}

bool deadMove (const char *reg, lineNode * currpl, lineNode * head)
{
    return FALSE;
}

bool notUsed (const char *reg, lineNode * currPl, lineNode * head)
{
    return FALSE;
}

bool canAssign (const char *op1, const char *op2, const char *op3)
{
    return FALSE;
}

bool notUsedFrom (const char *reg, const char *label, lineNode *head)
{
    return FALSE;
}

bool symmParmStack (void)
{
    return FALSE;
}
