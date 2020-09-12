#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef unsigned char u8;
typedef signed char i8;

typedef unsigned short u16;
typedef signed short i16;

typedef unsigned int u32;
typedef signed int i32;

typedef unsigned int b32;
typedef unsigned long long int b64;

typedef unsigned long long int u64;
typedef signed long long int i64;

typedef float f32;
typedef double f64;

#define internal static      //static function
#define global static        // global varibles
#define local_persist static //static local variables

#endif