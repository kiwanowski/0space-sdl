#include <string.h>

#include "engine.h"

World world;
ObjectVTable object_vtable[OBJ_MAX_ID];

bool object_is(int obj, int parent)
{
    if (obj < 0 || parent < 0) return false;
    while (obj >= 0 && obj < OBJ_MAX_ID) {
        if (obj == parent) return true;
        obj = object_parent[obj];
    }
    return false;
}

static Instance *instance_alloc(float x, float y, int obj)
{
    Instance *inst = NULL;
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (!world.instances[i].active) {
            inst = &world.instances[i];
            if (i >= world.instance_count) world.instance_count = i + 1;
            break;
        }
    }
    if (!inst) {
        SDL_Log("instance pool exhausted");
        return NULL;
    }

    memset(inst, 0, sizeof *inst);
    inst->active = true;
    inst->obj = obj;
    inst->id = ++world.next_id;
    inst->x = inst->xstart = inst->xprevious = x;
    inst->y = inst->ystart = inst->yprevious = y;
    inst->sprite_index = (obj >= 0 && obj < OBJ_MAX_ID) ? object_sprite[obj] : -1;
    inst->mask_index   = (obj >= 0 && obj < OBJ_MAX_ID) ? object_mask[obj] : -1;
    inst->depth = (obj >= 0 && obj < OBJ_MAX_ID) ? object_depth[obj] : 0;
    inst->solid = (obj >= 0 && obj < OBJ_MAX_ID) ? object_solid[obj] : false;
    inst->visible = (obj >= 0 && obj < OBJ_MAX_ID) ? object_visible[obj] : true;
    inst->image_speed = 1.0f;
    inst->image_xscale = inst->image_yscale = 1.0f;
    inst->image_alpha = 1.0f;
    inst->image_blend = 0xFFFFFF;
    for (int i = 0; i < 12; i++) inst->alarm[i] = -1;
    return inst;
}

Instance *instance_create(float x, float y, int obj)
{
    Instance *inst = instance_alloc(x, y, obj);
    if (inst && obj >= 0 && obj < OBJ_MAX_ID && object_vtable[obj].create)
        object_vtable[obj].create(inst);
    return inst;
}

void instance_destroy(Instance *inst)
{
    if (!inst || !inst->active) return;
    if (inst->obj >= 0 && inst->obj < OBJ_MAX_ID && object_vtable[inst->obj].destroy)
        object_vtable[inst->obj].destroy(inst);
    inst->active = false;
}

Instance *instance_find(int obj)
{
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (in->active && object_is(in->obj, obj)) return in;
    }
    return NULL;
}

int instance_number(int obj)
{
    int n = 0;
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (in->active && object_is(in->obj, obj)) n++;
    }
    return n;
}

Instance *instance_nearest(float x, float y, int obj)
{
    Instance *best = NULL;
    float bestd = 0;
    for (int i = 0; i < world.instance_count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || !object_is(in->obj, obj)) continue;
        float dx = in->x - x, dy = in->y - y;
        float d = dx * dx + dy * dy;
        if (!best || d < bestd) { best = in; bestd = d; }
    }
    return best;
}

void room_goto(int room)     { world.pending_room = room; }
void room_restart(void)      { world.restart_pending = true; }

void tile_layer_delete_at(int depth, float x, float y)
{
    const RoomDef *rd = &room_defs[world.room];
    for (int i = 0; i < rd->tile_count && i < MAX_TILES; i++) {
        const RoomTile *t = &rd->tiles[i];
        if (t->depth != depth) continue;
        if (x >= t->x && x < t->x + t->w && y >= t->y && y < t->y + t->h)
            world.tile_deleted[i] = 1;
    }
}

void world_load_room(int room)
{
    if (room < 0 || room >= ROOM_COUNT) return;
    const RoomDef *rd = &room_defs[room];

    for (int i = 0; i < world.instance_count; i++)
        world.instances[i].active = false;
    world.instance_count = 0;
    world.next_id = 0;

    world.room = room;
    world.room_width = rd->width;
    world.room_height = rd->height;
    world.view_w = rd->view_w;
    world.view_h = rd->view_h;
    world.hborder = rd->hborder;
    world.vborder = rd->vborder;
    world.view_x = 0;
    world.view_y = 0;
    world.clear_color = rd->clear_color;
    world.pending_room = -1;
    world.restart_pending = false;
    world.view_angle = 0.0f;
    world.camera_manual = false;
    world.hwrap = world.vwrap = false;
    world.view_count = 1;
    world.cam_x = rd->width / 2.0f;
    world.cam_y = rd->height / 2.0f;
    memset(world.tile_deleted, 0, sizeof world.tile_deleted);

    Instance *placed[MAX_INSTANCES];
    int placed_count = 0;

    for (int i = 0; i < rd->instance_count; i++) {
        const RoomInstance *ri = &rd->instances[i];
        Instance *in = instance_alloc((float)ri->x, (float)ri->y, ri->obj);
        if (in && placed_count < MAX_INSTANCES) placed[placed_count++] = in;
    }

    for (int i = 0, k = 0; i < rd->instance_count && k < placed_count; i++, k++) {
        const RoomInstance *ri = &rd->instances[i];
        Instance *in = placed[k];
        if (!in->active) continue;
        if (object_vtable[in->obj].create)
            object_vtable[in->obj].create(in);
        if (object_vtable[in->obj].set_var)
            for (int v = 0; v < ri->var_count; v++)
                object_vtable[in->obj].set_var(in, ri->vars[v].name, ri->vars[v].value);
    }

    for (int i = 0; i < placed_count; i++) {
        Instance *in = placed[i];
        if (in->active && object_vtable[in->obj].room_start)
            object_vtable[in->obj].room_start(in);
    }
}

static void view_follow(void)
{
    if (world.camera_manual) return;

    Instance *target = instance_find(OBJ_CHAR);
    if (target) {
        float cx = target->x, cy = target->y;
        float halfw = world.view_w / 2.0f, halfh = world.view_h / 2.0f;
        if (world.room_width  > world.view_w) {
            if (cx < halfw) cx = halfw;
            if (cx > world.room_width - halfw) cx = world.room_width - halfw;
        } else {
            cx = world.room_width / 2.0f;
        }
        if (world.room_height > world.view_h) {
            if (cy < halfh) cy = halfh;
            if (cy > world.room_height - halfh) cy = world.room_height - halfh;
        } else {
            cy = world.room_height / 2.0f;
        }
        world.cam_x = cx;
        world.cam_y = cy;
    } else {
        world.cam_x = world.room_width  / 2.0f;
        world.cam_y = world.room_height / 2.0f;
    }
}

void world_step(void)
{
    int count = world.instance_count;

    for (int i = 0; i < count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active) continue;
        in->xprevious = in->x;
        in->yprevious = in->y;
        if (object_vtable[in->obj].begin_step)
            object_vtable[in->obj].begin_step(in);
    }

    /* alarms */
    for (int i = 0; i < count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active) continue;
        for (int a = 0; a < 12; a++)
            if (in->alarm[a] >= 0) in->alarm[a]--;
    }

    for (int i = 0; i < count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active) continue;
        if (object_vtable[in->obj].step)
            object_vtable[in->obj].step(in);
    }

    for (int i = 0; i < count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active || in->sprite_index < 0) continue;
        in->image_index += in->image_speed;
        int n = sprite_defs[in->sprite_index].frame_count;
        if (n > 0) {
            while (in->image_index >= n) in->image_index -= n;
            while (in->image_index < 0)  in->image_index += n;
        }
    }

    for (int i = 0; i < count; i++) {
        Instance *in = &world.instances[i];
        if (!in->active) continue;
        if (object_vtable[in->obj].end_step)
            object_vtable[in->obj].end_step(in);
    }

    view_follow();

    if (world.restart_pending)      world_load_room(world.room);
    else if (world.pending_room >= 0) world_load_room(world.pending_room);
}
