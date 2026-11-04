/* ISC; see LICENSE. DOS console and native Glide bindings for sectorlisp. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "lisp.h"
#include "../common/glide_demo.h"
static int test_mode,quit;
static FILE *logfile;

static int init(const long *a,long *r) { (void)a; *r=0; return gfx_init(); }
static int close_graphics(const long *a,long *r) { (void)a; *r=0; gfx_close(); return 1; }
static int clear(const long *a,long *r) { (void)a; *r=0; return gfx_clear(); }
static int vertex(const long *a,long *r)
{
    *r=0;
    /* Lisp order: index x y red green blue. */
    return gfx_vertex((int)a[1],(int)a[2],(int)a[3],(int)a[4],(int)a[5],(int)a[0]);
}
static int triangle(const long *a,long *r)
{ *r=0; return gfx_triangle((int)a[0],(int)a[1],(int)a[2]); }
static int present(const long *a,long *r) { (void)a; *r=0; return gfx_swap(); }
static int key(const long *a,long *r) { (void)a; *r=test_mode?27:getch(); return 1; }
static int bye(const long *a,long *r) { (void)a; *r=0; quit=1; return 1; }
static void bindings(void)
{
    sl_bind("GLIDE-INIT",0,init); sl_bind("GLIDE-CLOSE",0,close_graphics);
    sl_bind("CLEAR",0,clear); sl_bind("VERTEX",6,vertex);
    sl_bind("TRIANGLE",3,triangle); sl_bind("PRESENT",0,present);
    sl_bind("KEY",0,key); sl_bind("BYE",0,bye);
}
static int load(const char *path)
{
    FILE *file=fopen(path,"rb");
    long size;
    char *source;
    int ok;
    if(!file) { printf("Cannot open %s\n",path); return 0; }
    fseek(file,0,SEEK_END); size=ftell(file); rewind(file);
    if(size<0 || size>262144) { fclose(file); return 0; }
    source=malloc(size+1);
    if(!source) { fclose(file); return 0; }
    if(fread(source,1,size,file)!=(size_t)size) { free(source); fclose(file); return 0; }
    fclose(file); source[size]=0;
    ok=sl_eval(source,0); free(source);
    if(!ok) { printf("%s: %s\n",path,sl_error()); if(logfile) fprintf(logfile,"%s: %s\n",path,sl_error()); }
    return ok;
}
static int expect_error(const char *source,const char *message)
{
    int ok=!sl_eval(source,0) && strstr(sl_error(),message)!=NULL;
    if(!ok && logfile) fprintf(logfile,"Expected error for %s; got %s\n",source,sl_error());
    return ok;
}
static int selftest(void)
{
    int ok=load("TESTS.LSP"),i,retained;
    char *stress;
    if(!expect_error("(car 1)","CAR requires")) ok=0;
    if(!expect_error("(cons 1)","Wrong number")) ok=0;
    if(!expect_error("(identity 1 2)","argument count")) ok=0;
    if(!expect_error("(assert nil)","Assertion failed")) ok=0;
    if(!expect_error("missing-symbol","Unbound symbol")) ok=0;
    if(!expect_error("(cons 1 . 2)","Dotted function")) ok=0;
    if(!expect_error("(quote 1 2)","Wrong number")) ok=0;
    if(!expect_error("(a b","Unclosed list")) ok=0;
    if(!expect_error(")","Unexpected closing")) ok=0;
    if(!expect_error("2147483648","32-bit range")) ok=0;
    if(!expect_error("(+ 2147483647 1)","overflow")) ok=0;
    if(!expect_error("(* -2147483648 -1)","overflow")) ok=0;
    if(!expect_error("(clear)","Native function failed")) ok=0;
    if(!sl_eval("(define alias 'alias)",0) || !expect_error("(alias)","step limit")) ok=0;
    if(!sl_eval("(define forever (lambda () (forever)))",0) ||
       !expect_error("(forever)","depth/step limit")) ok=0;
    sl_collect(); retained=sl_used();
    for(i=0;i<2000;++i) if(!sl_eval("(cons 'temporary '(a b c))",0)) { ok=0; break; }
    sl_collect(); if(sl_used()!=retained) ok=0;
    stress=malloc(40000);
    if(!stress) ok=0;
    else {
        stress[0]='\''; stress[1]='(';
        for(i=0;i<17000;++i) { stress[2+i*2]='1'; stress[3+i*2]=' '; }
        stress[34002]=')'; stress[34003]=0;
        if(!expect_error(stress,"Cons heap exhausted")) ok=0;
        for(i=0;i<150;++i) stress[i]='(';
        for(i=150;i<300;++i) stress[i]=')';
        stress[300]=0;
        if(!expect_error(stress,"Reader nesting")) ok=0;
        free(stress);
    }
    if(!sl_eval("(assert (eq (factorial 6) 720)) (assert (eq (car saved) 'persistent))",0)) ok=0;
    if(sl_complete("; ( ignored\n(cons 1\n") || !sl_complete("(cons 1\n '(2)) ; )\n")) ok=0;
    if(logfile) fprintf(logfile,"Lisp evaluator, recursion, GC, arithmetic and error recovery: %s\n",ok?"PASS":"FAIL");
    sl_reset(); return ok;
}
int main(int argc,char **argv)
{
    char line[1024],source[8192];
    size_t length=0,n;
    int ok=1;
    test_mode=argc>1 && !strcmp(argv[1],"/test");
    if(test_mode) {
        logfile=fopen("LISP.LOG","w"); if(!logfile) return 1;
        gd_begin_test(logfile);
    }
    sl_init(); bindings();
    puts("sectorlisp / Glide - 32-bit DOS");
    puts("RGB triangle defined in TRIANGLE.LSP. Press any key to return to Lisp.");
    if(test_mode && !selftest()) ok=0;
    if(!load(test_mode || argc==1?"TRIANGLE.LSP":argv[1])) ok=0;
    gfx_close();
    if(test_mode) {
        if(!sl_eval("(show-triangle)",0)) ok=0;
        if(!sl_eval("(show-triangle)",0)) ok=0;
        gfx_close();
        if(gd_captures()!=3 || gd_triangles()!=3 || gd_failed()) ok=0;
        fprintf(logfile,"%s: sectorlisp, 32-bit DOS, Voodoo Glide, RGB triangle (three displays)\n",ok?"PASS":"FAIL");
        fclose(logfile); gd_shutdown(); return ok?0:1;
    }
    puts("Ready. (show-triangle) draws again; (bye) exits; (reset) clears definitions.");
    source[0]=0;
    while(!quit) {
        printf(length?"... ":"* ");
        if(!fgets(line,sizeof(line),stdin)) break;
        if(!length && (!strcmp(line,"(reset)\n") || !strcmp(line,"(RESET)\n"))) {
            gfx_close(); sl_reset(); continue;
        }
        n=strlen(line);
        if(length+n>=sizeof(source)) { puts("Input exceeds 8191 bytes; discarded"); length=0; source[0]=0; continue; }
        memcpy(source+length,line,n+1); length+=n;
        if(!sl_complete(source)) continue;
        if(!sl_eval(source,1)) { gfx_close(); printf("error: %s\n",sl_error()); }
        length=0; source[0]=0;
    }
    gd_shutdown(); return 0;
}
