#ifndef NAVE_SCENE_H
#define NAVE_SCENE_H
#include "../textures/model.h"
typedef struct { Vec p[4],normal,center; } GlassWindow;
extern GlassWindow glass_windows[24];
extern int glass_count;
extern const Vec candle_positions[12];
#endif
