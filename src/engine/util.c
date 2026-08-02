#include <math.h>
#include <stdlib.h>

#include "engine.h"

#define DEG2RAD 0.017453292519943295

float gm_lengthdir_x(float len, float dir)
{
    return (float)((double)len * cos((double)dir * DEG2RAD));
}

float gm_lengthdir_y(float len, float dir)
{
    return (float)(-(double)len * sin((double)dir * DEG2RAD));
}

float gm_point_direction(float x1, float y1, float x2, float y2)
{
    double d = atan2((double)y1 - y2, (double)x2 - x1) / DEG2RAD;
    if (d < 0) d += 360.0;
    return (float)d;
}

float gm_point_distance(float x1, float y1, float x2, float y2)
{
    double dx = (double)x2 - x1, dy = (double)y2 - y1;
    return (float)sqrt(dx * dx + dy * dy);
}

int gm_round(float v) { return (int)nearbyintf(v); }

int gm_irandom(int n)
{
    if (n <= 0) return 0;
    return rand() % (n + 1);
}

float gm_random(float n)
{
    return (float)rand() / (float)RAND_MAX * n;
}

float gm_sign(float v)
{
    return v > 0 ? 1.0f : (v < 0 ? -1.0f : 0.0f);
}
