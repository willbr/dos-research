/* DOS/Glide host for the 32-bit sectorforth port. No geometry is defined here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <conio.h>
#include "../common/glide_demo.h"

extern int __cdecl sf_eval(char *source);
extern void __cdecl sf_reset(void);
extern int __cdecl sf_depth(void);
extern int __cdecl sf_compiling(void);
static int test_mode, failure;
static FILE *logfile;

int __cdecl host_number(const char *text, int length, long *value)
{
    unsigned long n=0, limit;
    int i=0, negative=0, base=10, digit;
    if (!length) return 0;
    if (text[i]=='-') { negative=1; if (++i==length) return 0; }
    if (text[i]=='$') { base=16; if (++i==length) return 0; }
    limit=negative?2147483648UL:2147483647UL;
    for (;i<length;++i) {
        digit=text[i]>='0' && text[i]<='9' ? text[i]-'0' :
              text[i]>='a' && text[i]<='f' ? text[i]-'a'+10 : -1;
        if (digit<0 || digit>=base || n>(limit-digit)/base) return 0;
        n=n*base+digit;
    }
    *value=negative ? (long)(0UL-n) : (long)n;
    return 1;
}
void __cdecl host_emit(int c) { putchar(c&255); }
int __cdecl host_key(void) { return test_mode?27:getch(); }
void __cdecl host_dot(long n) { printf("%ld ",n); }

static int evaluate(const char *text)
{
    char *copy=malloc(strlen(text)+1);
    int result;
    if (!copy) return 5;
    strcpy(copy,text); result=sf_eval(copy); free(copy);
    return result;
}
static int load(const char *path)
{
    FILE *file=fopen(path,"rb");
    long size;
    char *source;
    int result;
    if (!file) { printf("Cannot open %s\n",path); return 1; }
    fseek(file,0,SEEK_END); size=ftell(file); rewind(file);
    if (size<0 || size>262144) { fclose(file); return 1; }
    source=malloc(size+1);
    if (!source) { fclose(file); return 1; }
    if (fread(source,1,size,file)!=(size_t)size) { free(source); fclose(file); return 1; }
    fclose(file); source[size]=0;
    result=sf_eval(source);
    if (!result && sf_compiling()) result=4;
    free(source);
    return result;
}
static int selftest(void)
{
    int ok=1,i;
    char *stress;
    sf_reset();
    if (load("TESTS.F") || sf_depth()) ok=0;
    if (evaluate("not-a-word")!=1 || sf_depth()) ok=0;
    if (evaluate("drop")!=2) ok=0;
    if (evaluate("0 assert")!=6) ok=0;
    if (evaluate(": broken unknown")!=1 || sf_compiling()) ok=0;
    if (evaluate("broken")!=1) ok=0;
    if (evaluate("1") || evaluate("2 + 3 = assert") || sf_depth()) ok=0;
    if (evaluate(": across") || !sf_compiling() || evaluate("7 ; across 7 = assert")) ok=0;
    if (evaluate("vertex!")!=2 || evaluate("1 2 vertex!")!=2 ||
        evaluate("6 7 + 13 = assert")) ok=0;
    if (evaluate("2147483648")!=1) ok=0;
    if (evaluate("clear")!=7) ok=0;
    stress=malloc(70000);
    if (!stress) ok=0;
    else {
        for (i=0;i<1025;++i) { stress[i*2]='1'; stress[i*2+1]=' '; }
        stress[2050]=0;
        if (evaluate(stress)!=3 || sf_depth()) ok=0;
        strcpy(stress,": huge ");
        for (i=0;i<32768;++i) { stress[7+i*2]='1'; stress[8+i*2]=' '; }
        stress[65543]=0;
        if (evaluate(stress)!=5 || sf_compiling() || evaluate("huge")!=1) ok=0;
        free(stress);
    }
    if (evaluate("six 6 = assert") || sf_depth()) ok=0;
    if (logfile) fprintf(logfile,"Forth compiler, stacks, numbers and recovery: %s\n",ok?"PASS":"FAIL");
    sf_reset();
    return ok;
}
int main(int argc,char **argv)
{
    char line[1024];
    int result;
    test_mode=argc>1 && strcmp(argv[1],"/test")==0;
    if (test_mode) { logfile=fopen("FORTH.LOG","w"); if (!logfile) return 1; gd_begin_test(logfile); }
    puts("sectorforth / Glide - 32-bit DOS");
    puts("RGB triangle defined in TRIANGLE.F. Press a key to return to Forth.");
    if (test_mode && !selftest()) failure=1;
    result=load(test_mode || argc==1 ? "TRIANGLE.F" : argv[1]);
    gfx_close();
    if (result) { printf("Forth error %d\n",result); failure=1; }
    if (test_mode) {
        /* Exercise the same compiled word again after returning to text. */
        if (evaluate("show-triangle")) failure=1;
        if (evaluate("show-triangle")) failure=1;
        gfx_close();
        if (gd_captures()!=3 || gd_triangles()!=3 || gd_failed() || sf_depth()) failure=1;
        if (logfile) {
            fprintf(logfile,"%s: sectorforth, 32-bit DOS, Voodoo Glide, RGB triangle\n",failure?"FAIL":"PASS");
            fclose(logfile);
        }
        gd_shutdown();
        return failure?1:0;
    }
    puts("Ready. 'show-triangle' draws again; 'bye' exits. 'reset' clears the dictionary.");
    for (;;) {
        printf("\n%s> ",sf_compiling()?"compile":"ok");
        if (!fgets(line,sizeof(line),stdin)) break;
        if (!strcmp(line,"bye\n")) break;
        if (!strcmp(line,"reset\n")) { gfx_close(); sf_reset(); continue; }
        result=sf_eval(line);
        if (result) { gfx_close(); printf("error %d (stacks reset; incomplete definition discarded)\n",result); }
    }
    gd_shutdown();
    return 0;
}
