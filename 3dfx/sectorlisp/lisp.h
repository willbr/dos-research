/* ISC; see LICENSE. Native functions receive checked signed integer arguments. */
#ifndef SECTORLISP_PORT_H
#define SECTORLISP_PORT_H
typedef int (*SlNative)(const long *args,long *result);
void sl_init(void);
void sl_reset(void);
int sl_bind(const char *name,int arity,SlNative fn);
int sl_eval(const char *source,int print_results);
const char *sl_error(void);
int sl_complete(const char *source);
int sl_used(void);
void sl_collect(void);
#endif
