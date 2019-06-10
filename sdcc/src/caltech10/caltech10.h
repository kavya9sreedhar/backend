typedef enum
  {
    SUB_CALTECH10
  }
CALTECH10_SUB_PORT;

typedef struct
  {
    CALTECH10_SUB_PORT sub;
  }
CALTECH10_OPTS;

extern CALTECH10_OPTS hc08_opts;

#define IS_CALTECH10 (caltech10_opts.sub == SUB_CALTECH10)
