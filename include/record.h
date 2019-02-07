#ifndef _GSYSMON_RECORD_H
#define _GSYSMON_RECORD_H

#include <stdlib.h>
#include <gtk/gtk.h>

enum record_type_t {
    RECORD_TYPE_CHAR,
    RECORD_TYPE_INT,
    RECORD_TYPE_DOUBLE
};

struct record {
    size_t              cap;    // capacity
    enum record_type_t  dtype;  // data type
    GQueue             *data;   // actual data holder
};

/**
 * record_new() - creates a new record struct
 * @cap: maximum capacity of record database
 * @dtype: data type of storage
 *
 * record_new() returns a newly allocated record struct.
 */
struct record *record_new(size_t cap, enum record_type_t dtype);

/** record_destroy_with_data() - destroy @rec, along with its containing data
 * @rec
 */
void record_destroy_with_data(struct record *rec);

/**
 * record_get_capacity() - helper function to get @rec's capacity
 * @rec
 */
static inline size_t record_get_capacity(struct record *rec) {
    return rec->cap;
}

/**
 * record_get_length() - gets @rec's currently stored elements count
 * @rec
 *
 * record_get_length() returns number elements stored.
 */
static inline size_t record_get_length(struct record *rec) {
    return (rec->data != NULL) ? g_queue_get_length(rec->data) : 0;
}

/**
 * record_is_overflowing() - checks whether @rec is overflowing
 * @rec
 */
static inline int record_is_overflowing(struct record *rec) {
    return (rec->cap == record_get_length(rec));
}

/**
 * record_push_discard_overflow() - pushes an allocated data glob to @rec
 * @rec
 * @data_alloced
 *
 * record_push_discard_overflow() pushed data pointer @data_alloced to the
 * queue, and discards data if length of queue is exceeding capacity. The
 * overflowed data will be free()-d.
 *
 * record_push_discard_overflow() returns a non-zero value, see
 * record_is_overflowing(), if @rec will overflow on next insertion.
 */
int record_push_discard_overflow(struct record *rec, void *data_alloced);

static inline int record_push_lf(struct record *rec, double d) {
    double *cell = malloc(sizeof(double));
    *cell = d;
    return record_push_discard_overflow(rec, cell);
}

/**
 * [BUILDING]
 * record_render() - renders data in @rec to @cr
 * @rec
 * @cr
 * @w: width of drawing area
 * @h: height of drawing area
 */
void record_render(struct record *rec, cairo_t *cr, size_t w, size_t h);

#endif

