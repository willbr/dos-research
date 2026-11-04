/* SPDX-License-Identifier: MIT - shared DOS/Glide demo bridge. */
#ifndef GLIDE_DEMO_H
#define GLIDE_DEMO_H
#include <stdio.h>
int __cdecl gfx_init(void);
void __cdecl gfx_close(void);
int __cdecl gfx_clear(void);
int __cdecl gfx_vertex(int x,int y,int r,int g,int b,int index);
int __cdecl gfx_triangle(int a,int b,int c);
int __cdecl gfx_swap(void);
void gd_shutdown(void);
void gd_begin_test(FILE *log);
int gd_captures(void);
int gd_triangles(void);
int gd_failed(void);
#endif
