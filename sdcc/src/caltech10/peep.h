#ifndef PEEPC10_INCLUDE
#define PEEPC10_INCLUDE
//more files to inc?

int getSize (lineNode * line);
bitVect getRegsRead (lineNode * line);
bitVect getRegsWritten (lineNode * line);
bool deadMove (const char *reg, lineNode * currpl, lineNode * head);
bool notUsed (const char *reg, lineNode * currPl, lineNode * head);
bool canAssign (const char *op1, const char *op2, const char *op3);
bool notUsedFrom (const char *reg, const char *label, lineNode *head);
bool symmParmStack (void);

#endif
