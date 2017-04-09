/**
 * @Author: ToSeven
 * @Date:   2017-04-07T16:09:36+08:00
 * @Email:  byone.heng@gmail.com
 * @Last modified by:   ToSeven
 * @Last modified time: 2017-04-07T20:12:59+08:00
 * @License: MIT
 */

#ifndef _DISPATCH_H_
#define _DISPATCH_H_

typedef struct _operation_t operation_t;
typedef struct _dispatch_queue_t dispatch_queue_t;

struct _operation_t {
    operation_t* next;
    function_t func;
    void* args;
    uint32_t timestamp;
};

struct _dispatch_queue_t {
    operation_t *head, *tail;
};

void dispatch_init( void );
void dispatch( void );

#endif // _DISPATCH_H_
