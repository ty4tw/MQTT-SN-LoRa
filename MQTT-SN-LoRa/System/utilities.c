/*!
 * \file      utilities.h
 *
 * \brief     Helper functions implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "utilities.h"

uint8_t theTraceFlg = 0;

void SetTraceMode(uint8_t flg)
{
	theTraceFlg = flg;
}


/*!
 * Redefinition of rand() and srand() standard C functions.
 * These functions are redefined in order to get the same behavior across
 * different compiler toolchains implementations.
 */
// Standard random functions redefinition start
#define RAND_LOCAL_MAX 2147483647L

static uint32_t next = 1;

int32_t rand1( void )
{
    return ( ( next = next * 1103515245L + 12345L ) % RAND_LOCAL_MAX );
}

void srand1( uint32_t seed )
{
    next = seed;
}
// Standard random functions redefinition end

int32_t randr( int32_t min, int32_t max )
{
    return ( int32_t )rand1( ) % ( max - min + 1 ) + min;
}

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    while( size-- )
    {
        *dst++ = *src++;
    }
}

void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    dst = dst + ( size - 1 );
    while( size-- )
    {
        *dst-- = *src++;
    }
}

void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    while( size-- )
    {
        *dst++ = value;
    }
}

int8_t Nibble2HexChar( uint8_t a )
{
    if( a < 10 )
    {
        return '0' + a;
    }
    else if( a < 16 )
    {
        return 'A' + ( a - 10 );
    }
    else
    {
        return '?';
    }
}

uint16_t getUint16(const uint8_t* pos){
    uint16_t val = ((uint16_t)*pos++ << 8);
    return val += *pos;
}

void setUint16(uint8_t* pos, uint16_t val){
    *pos++ = (val >> 8) & 0xff;
    *pos   = val & 0xff;
}

uint32_t getUint32(const uint8_t* pos){
    uint32_t val = (uint32_t)(*pos++) <<  24;
    val += (uint32_t)(*pos++) << 16;
    val += (uint32_t)(*pos++) <<  8;
    return val += *pos++;
}

uint32_t getUint24(const uint8_t* pos)
{
    uint32_t val = 0;
    val += (uint32_t)(*pos++) << 16;
    val += (uint32_t)(*pos++) <<  8;
    return val += *pos++;
}

void setUint32(uint8_t* pos, uint32_t val){
    *pos++ = (val >> 24) & 0xff;
    *pos++ = (val >> 16) & 0xff;
    *pos++ = (val >>  8) & 0xff;
    *pos   =  val & 0xff;
}

float getFloat32(const uint8_t* pos){
    union{
        float flt;
        uint8_t d[4];
    }val;
    val.d[3] = *pos++;
    val.d[2] = *pos++;
    val.d[1] = *pos++;
    val.d[0] = *pos;
    return val.flt;
}

void setFloat32(uint8_t* pos, float flt){
    union{
        float flt;
        uint8_t d[4];
    }val;
    val.flt = flt;
    *pos++ = val.d[3];
    *pos++ = val.d[2];
    *pos++ = val.d[1];
    *pos   = val.d[0];
}

/**
 *  Calculate free SRAM
 */
int GetFreeRam(void)
{
    extern char __bss_end__;

    int freeMemory;

    freeMemory = ((int)&freeMemory) - ((int)&__bss_end__);
    return freeMemory;
}

