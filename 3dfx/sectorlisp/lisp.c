/* DOS port derived from jart/sectorlisp's C reference evaluator.
 * Copyright 2020 Justine Alexandra Roberts Tunney
 * Copyright 2026 dos-research contributors
 * ISC license; see LICENSE and upstream/LICENSE.
 *
 * Retains Car/Cdr/Cons, Evlis/Pairlis/Assoc/Evcon/Apply/Eval and the
 * negative-cons/nonnegative-atom representation. Stable cons handles and
 * collection between top-level forms replace upstream's per-Eval copying
 * collector so DEFINE can retain objects safely across REPL evaluations.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <setjmp.h>
#include "lisp.h"

#define CELLS 16384
#define ATOM_BYTES 131072
#define NATIVE_MAX 32
typedef int Obj;
typedef char Require32BitCells[(sizeof(int)==4 && sizeof(long)==4)?1:-1];
static Obj heap[CELLS][2], globals;
static unsigned char used[CELLS], marked[CELLS];
static int free_head, live, atom_end;
static char atoms[ATOM_BYTES], error_message[160];
static const char *input;
static jmp_buf escape;
static int eval_depth, read_depth, steps;
static Obj kT,kQuote,kCond,kRead,kPrint,kAtom,kCar,kCdr,kCons,kEq;
static Obj kLambda,kLabel,kDefine,kProgn,kPlus,kMinus,kTimes,kLess,kAssert;
static struct { Obj symbol; int arity; SlNative fn; } natives[NATIVE_MAX];
static int native_count;

static Obj Read(void), Eval(Obj,Obj), Apply(Obj,Obj,Obj);
static void Fail(const char *message)
{
    strncpy(error_message,message,sizeof(error_message)-1);
    error_message[sizeof(error_message)-1]=0;
    longjmp(escape,1);
}
static Obj Intern(const char *text)
{
    char name[64];
    int i,n=(int)strlen(text);
    if (n>63) Fail("Atom name exceeds 63 characters");
    for(i=0;i<n;++i) name[i]=(char)toupper((unsigned char)text[i]);
    name[n]=0;
    if (!strcmp(name,"NIL")) return 0;
    for(i=1;i<atom_end;i+=(int)strlen(atoms+i)+1)
        if (!strcmp(atoms+i,name)) return i;
    if (atom_end+n+1>ATOM_BYTES) Fail("Atom storage exhausted; restart the program");
    i=atom_end; memcpy(atoms+i,name,n+1); atom_end+=n+1;
    return i;
}
static const char *Name(Obj x) { return x?atoms+x:"NIL"; }
static int IsNumber(Obj x,long *value)
{
    const char *name;
    char *end;
    long n;
    if (x<=0) return 0;
    name=Name(x);
    if (!isdigit((unsigned char)*name) && !(*name=='-' && isdigit((unsigned char)name[1]))) return 0;
    errno=0; n=strtol(name,&end,10);
    if (*end) return 0;
    if (errno==ERANGE) Fail("Integer outside signed 32-bit range");
    *value=n; return 1;
}
static Obj Number(long n)
{
    char text[32]; sprintf(text,"%ld",n); return Intern(text);
}
static long Integer(Obj x)
{
    long n;
    if (!IsNumber(x,&n)) Fail("Expected an integer");
    return n;
}
static Obj Car(Obj x)
{
    if (!x) return 0;
    if (x>0) Fail("CAR requires a list");
    return heap[-x-1][0];
}
static Obj Cdr(Obj x)
{
    if (!x) return 0;
    if (x>0) Fail("CDR requires a list");
    return heap[-x-1][1];
}
static Obj Cons(Obj car,Obj cdr)
{
    int slot=free_head;
    if (slot<0) Fail("Cons heap exhausted within one expression");
    free_head=heap[slot][1]; used[slot]=1; ++live;
    heap[slot][0]=car; heap[slot][1]=cdr;
    return -slot-1;
}
static int Length(Obj x)
{
    int n=0;
    while(x) { if(x>0) Fail("Expected a proper argument list"); ++n; x=Cdr(x); }
    return n;
}
static void Arity(Obj args,int n)
{
    if(Length(args)!=n) Fail("Wrong number of arguments");
}
static Obj Assoc(Obj x,Obj env)
{
    while(env) { Obj binding=Car(env); if(x==Car(binding)) return binding; env=Cdr(env); }
    return 0;
}
static Obj Lookup(Obj x,Obj env)
{
    Obj binding=Assoc(x,env);
    if(!binding) binding=Assoc(x,globals);
    if(!binding) {
        char message[100]; sprintf(message,"Unbound symbol: %.63s",Name(x)); Fail(message);
    }
    return Cdr(binding);
}
static void Define(Obj name,Obj value)
{
    Obj binding;
    long n;
    if(name<=0 || name==kT || IsNumber(name,&n)) Fail("DEFINE requires a nonnumeric symbol");
    binding=Assoc(name,globals);
    if(binding) heap[-binding-1][1]=value;
    else globals=Cons(Cons(name,value),globals);
}
static void Skip(void)
{
    for(;;) {
        while(*input && isspace((unsigned char)*input)) ++input;
        if(*input!=';') return;
        while(*input && *input!='\n') ++input;
    }
}
static int Delimiter(char c)
{
    return !c || isspace((unsigned char)c) || c=='(' || c==')' || c=='\'' || c==';';
}
static Obj GetObject(void)
{
    Obj first=0,last=0,node,value;
    char text[64];
    int n=0;
    long number;
    Skip();
    if(!*input) Fail("Unexpected end of input");
    if(*input=='\'') {
        ++input; value=Read(); return Cons(kQuote,Cons(value,0));
    }
    if(*input==')') Fail("Unexpected closing parenthesis");
    if(*input=='(') {
        ++input;
        for(;;) {
            Skip();
            if(!*input) Fail("Unclosed list");
            if(*input==')') { ++input; return first; }
            if(*input=='.' && Delimiter(input[1])) {
                if(!last) Fail("Dot before list element");
                ++input; value=Read(); Skip();
                if(*input!=')') Fail("Dotted list needs one tail");
                ++input; heap[-last-1][1]=value; return first;
            }
            value=Read(); node=Cons(value,0);
            if(last) heap[-last-1][1]=node; else first=node;
            last=node;
        }
    }
    while(!Delimiter(*input)) {
        if(n==63) Fail("Atom name exceeds 63 characters");
        text[n++]=*input++;
    }
    text[n]=0;
    if(!strcmp(text,".")) Fail("Unexpected dot");
    value=Intern(text);
    return IsNumber(value,&number)?Number(number):value;
}
static Obj Read(void)
{
    Obj result;
    if(++read_depth>128) Fail("Reader nesting limit exceeded");
    result=GetObject(); --read_depth; return result;
}
static void PrintObject(Obj x,int depth)
{
    if(depth>128) { fputs("#<depth-limit>",stdout); return; }
    if(x>=0) { fputs(Name(x),stdout); return; }
    putchar('(');
    for(;;) {
        PrintObject(Car(x),depth+1); x=Cdr(x);
        if(!x) break;
        if(x>0) { fputs(" . ",stdout); PrintObject(x,depth+1); break; }
        putchar(' ');
    }
    putchar(')');
}
static Obj Evlis(Obj args,Obj env)
{
    Obj first=0,last=0,value,node;
    while(args) {
        if(args>0) Fail("Dotted function arguments");
        value=Eval(Car(args),env); node=Cons(value,0);
        if(last) heap[-last-1][1]=node; else first=node;
        last=node; args=Cdr(args);
    }
    return first;
}
static Obj Pairlis(Obj names,Obj values,Obj env)
{
    if(Length(names)!=Length(values)) Fail("Lambda argument count mismatch");
    while(names) {
        Obj name=Car(names); long n;
        if(name<=0 || name==kT || IsNumber(name,&n)) Fail("Invalid lambda parameter");
        env=Cons(Cons(name,Car(values)),env);
        names=Cdr(names); values=Cdr(values);
    }
    return env;
}
static Obj Sequence(Obj body,Obj env)
{
    Obj result=0;
    while(body) { result=Eval(Car(body),env); body=Cdr(body); }
    return result;
}
static Obj Evcon(Obj clauses,Obj env)
{
    while(clauses) {
        Obj clause=Car(clauses);
        if(Length(clause)<2) Fail("COND clause requires a test and body");
        if(Eval(Car(clause),env)) return Sequence(Cdr(clause),env);
        clauses=Cdr(clauses);
    }
    return 0;
}
static Obj Apply(Obj f,Obj args,Obj env)
{
    int i,j;
    long a,b,result=0,values[8];
dispatch:
    if(++steps>100000) Fail("Evaluation step limit exceeded");
    if(f<0) {
        Obj kind=Car(f),parameters,body;
        if(kind==kLabel) {
            Obj name,fn;
            Arity(Cdr(f),2);
            name=Car(Cdr(f)); fn=Car(Cdr(Cdr(f)));
            if(name<=0) Fail("LABEL requires a symbol");
            env=Cons(Cons(name,f),env);
            f=fn; goto dispatch;
        }
        if(kind!=kLambda || Length(f)<3) Fail("Expected LAMBDA or LABEL function");
        parameters=Car(Cdr(f)); body=Cdr(Cdr(f));
        return Sequence(body,Pairlis(parameters,args,env));
    }
    if(f==kEq) { Arity(args,2); return Car(args)==Car(Cdr(args))?kT:0; }
    if(f==kCons) { Arity(args,2); return Cons(Car(args),Car(Cdr(args))); }
    if(f==kAtom) { Arity(args,1); return Car(args)<0?0:kT; }
    if(f==kCar) { Arity(args,1); return Car(Car(args)); }
    if(f==kCdr) { Arity(args,1); return Cdr(Car(args)); }
    if(f==kRead) { Arity(args,0); return Read(); }
    if(f==kPrint) {
        if(Length(args)>1) Fail("PRINT takes zero or one argument");
        if(args) PrintObject(Car(args),0); putchar('\n'); return 0;
    }
    if(f==kAssert) { Arity(args,1); if(!Car(args)) Fail("Assertion failed"); return kT; }
    if(f==kPlus || f==kMinus || f==kTimes || f==kLess) {
        Arity(args,2); a=Integer(Car(args)); b=Integer(Car(Cdr(args)));
        if(f==kLess) return a<b?kT:0;
        if(f==kPlus) {
            if((b>0 && a>LONG_MAX-b) || (b<0 && a<LONG_MIN-b)) Fail("Integer overflow");
            result=a+b;
        } else if(f==kMinus) {
            if((b<0 && a>LONG_MAX+b) || (b>0 && a<LONG_MIN+b)) Fail("Integer overflow");
            result=a-b;
        } else {
            if((a>0 && ((b>0 && a>LONG_MAX/b) || (b<0 && b<LONG_MIN/a))) ||
               (a<0 && ((b>0 && a<LONG_MIN/b) || (b<0 && a<LONG_MAX/b)))) Fail("Integer overflow");
            result=a*b;
        }
        return Number(result);
    }
    for(i=0;i<native_count;++i) if(f==natives[i].symbol) {
        Obj p=args;
        Arity(args,natives[i].arity);
        for(j=0;j<natives[i].arity;++j) { values[j]=Integer(Car(p)); p=Cdr(p); }
        if(!natives[i].fn(values,&result)) Fail("Native function failed (graphics state or argument)");
        return Number(result);
    }
    f=Lookup(f,env);
    goto dispatch;
}
static Obj Eval(Obj expression,Obj env)
{
    Obj result,head,args,value;
    long number;
    if(++eval_depth>128 || ++steps>100000) Fail("Evaluation depth/step limit exceeded");
    if(expression>=0) {
        result=(!expression || expression==kT || IsNumber(expression,&number))?
            expression:Lookup(expression,env);
    } else {
        head=Car(expression); args=Cdr(expression);
        if(head==kQuote) { Arity(args,1); result=Car(args); }
        else if(head==kCond) result=Evcon(args,env);
        else if(head==kLambda || head==kLabel) result=expression;
        else if(head==kProgn) result=Sequence(args,env);
        else if(head==kDefine) {
            Arity(args,2); value=Eval(Car(Cdr(args)),env);
            Define(Car(args),value); result=Car(args);
        } else {
            if(head<0 && Car(head)!=kLambda && Car(head)!=kLabel) head=Eval(head,env);
            result=Apply(head,Evlis(args,env),env);
        }
    }
    --eval_depth; return result;
}

void sl_collect(void)
{
    static int work[CELLS];
    int top=0,i,slot;
    Obj x;
    memset(marked,0,sizeof(marked));
    if(globals<0) { slot=-globals-1; marked[slot]=1; work[top++]=slot; }
    while(top) {
        slot=work[--top];
        for(i=0;i<2;++i) {
            x=heap[slot][i];
            if(x<0 && !marked[-x-1]) { marked[-x-1]=1; work[top++]=-x-1; }
        }
    }
    free_head=-1; live=0;
    for(i=CELLS-1;i>=0;--i) {
        if(used[i] && marked[i]) ++live;
        else { used[i]=0; heap[i][1]=free_head; free_head=i; }
    }
}
void sl_reset(void)
{
    globals=0; memset(used,0,sizeof(used)); sl_collect();
}
void sl_init(void)
{
    atom_end=1; native_count=0;
    kT=Intern("T"); kQuote=Intern("QUOTE"); kCond=Intern("COND");
    kRead=Intern("READ"); kPrint=Intern("PRINT"); kAtom=Intern("ATOM");
    kCar=Intern("CAR"); kCdr=Intern("CDR"); kCons=Intern("CONS"); kEq=Intern("EQ");
    kLambda=Intern("LAMBDA"); kLabel=Intern("LABEL"); kDefine=Intern("DEFINE");
    kProgn=Intern("PROGN"); kPlus=Intern("+"); kMinus=Intern("-");
    kTimes=Intern("*"); kLess=Intern("<"); kAssert=Intern("ASSERT");
    sl_reset();
}
int sl_bind(const char *name,int arity,SlNative fn)
{
    if(native_count==NATIVE_MAX || arity<0 || arity>8) return 0;
    natives[native_count].symbol=Intern(name);
    natives[native_count].arity=arity; natives[native_count++].fn=fn;
    return 1;
}
int sl_eval(const char *source,int print_results)
{
    Obj form,result;
    input=source; error_message[0]=0; eval_depth=read_depth=0;
    if(setjmp(escape)) { eval_depth=read_depth=0; sl_collect(); return 0; }
    for(;;) {
        Skip(); if(!*input) return 1;
        sl_collect(); steps=0;
        form=Read(); result=Eval(form,0);
        if(print_results) { PrintObject(result,0); putchar('\n'); }
    }
}
const char *sl_error(void) { return error_message; }
int sl_used(void) { return live; }
int sl_complete(const char *source)
{
    int nesting=0,comment=0;
    for(;*source;++source) {
        if(comment) { if(*source=='\n') comment=0; continue; }
        if(*source==';') { comment=1; continue; }
        if(*source=='(') ++nesting;
        if(*source==')' && --nesting<0) return 1;
    }
    return nesting==0;
}
