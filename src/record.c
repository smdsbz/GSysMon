#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "../include/record.h"


struct record *record_new(size_t cap, enum record_type_t dtype) {
    struct record *rec = malloc(sizeof(struct record));
    if (rec == NULL) {
        perror("malloc");
        return NULL;
    }
    memset(rec, 0, sizeof(struct record));
    rec->cap = cap;
    rec->dtype = dtype;
    rec->data = g_queue_new();
    return rec;
}

void record_destroy_with_data(struct record *rec) {
    if (rec->data != NULL) {
        g_queue_free_full(rec->data, free);
    }
    free(rec);
    return;
}

int record_push_discard_overflow(struct record *rec, void *data_alloced) {
    g_queue_push_head(rec->data, data_alloced);
    while (g_queue_get_length(rec->data) > rec->cap) {
        free(g_queue_pop_tail(rec->data));
    }
    return record_is_overflowing(rec);
}

/**
 *                      @width * (@idx_right / @cap)
 *                0             ,-+--.
 *                  ,-----------o----+---->  x-axis
 *                  |                |
 *                  |           |    |
 *                  |                |   @height
 *                 ,o - - - - - x    |
 * @height * @perc +|                |
 *                 `+----------------'
 *                  |
 *                  |     @width
 *                  v
 *
 *                  y-axis
 */
static inline size_t get_x(size_t idx_right, size_t cap, size_t width) {
    return (size_t)(((double)1.0 - ((double)idx_right / (double)cap)) * (double)width);
}
static inline size_t get_y(double perc, size_t height) {
    return (size_t)(((double)1.0 - perc) * (double)height);
}

void record_render(struct record *rec, cairo_t *cr, size_t w, size_t h) {
    // draw outer border
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, 1, 1);
    cairo_line_to(cr, w - 1, 1);
    cairo_line_to(cr, w - 1, h - 1);
    cairo_line_to(cr, 1, h - 1);
    cairo_line_to(cr, 1, 1);
    // draw internal helper lines
    for (double hor = 0.25; hor < 0.8; hor += 0.25) {
        cairo_move_to(cr, 0, get_y(hor, h));
        cairo_line_to(cr, w - 1, get_y(hor, h));
    }
    cairo_stroke(cr);
    // draw content, from right to left, from latest to earliest
    GList *cursor = rec->data->head;
    if (cursor == NULL) {
        return;
    }
    cairo_set_source_rgb(cr, 0.3, 0.8, 0.7);
    cairo_set_line_width(cr, 2.0);
    size_t idx = 0;
    cairo_move_to(cr, get_x(idx++, rec->cap, w), get_y(*(double *)cursor->data, h));
    for (cursor = cursor->next; cursor != NULL; cursor = cursor->next, ++idx) {
        cairo_line_to(cr, get_x(idx, rec->cap, w), get_y(*(double *)cursor->data, h));
    }
    // finish drawing
    cairo_stroke(cr);
    return;
}

/******* Test *******/

#if GSYSMON_RECORD_TEST
int main(const int argc, const char **argv) {

    cairo_surface_t *surface;
    cairo_t *cr;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 390, 60);
    cr = cairo_create(surface);

    struct record *rec = record_new(10, RECORD_TYPE_DOUBLE);
    record_push_lf(rec, 0.1);
    record_push_lf(rec, 0.2);
    record_push_lf(rec, 0.7);
    record_push_lf(rec, 0.5);
    record_render(rec, cr, 390, 60);

    cairo_surface_write_to_png(surface, "image.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return 0;
}
#endif

