/*!
 * \file      Payload.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/
#include "Payload.h"
#include <stdio.h>
#include <string.h>
#include "utilities.h"


/**
 *  Big Endian convert
 */
uint16_t getUint16From(const uint8_t* pos){
    uint16_t val = ((uint16_t)*pos++ << 8);
    return val += *pos;
}

void setUint16To(uint8_t* pos, uint16_t val){
    *pos++ = (val >> 8) & 0xff;
    *pos   = val & 0xff;
}

uint32_t getUint32From(const uint8_t* pos){
    uint32_t val = (uint32_t)(*pos++) <<  24;
    val += (uint32_t)(*pos++) << 16;
    val += (uint32_t)(*pos++) <<  8;
    return val += *pos++;
}

void setUint32To(uint8_t* pos, uint32_t val){
    *pos++ = (val >> 24) & 0xff;
    *pos++ = (val >> 16) & 0xff;
    *pos++ = (val >>  8) & 0xff;
    *pos   =  val & 0xff;
}

float getFloat32From(const uint8_t* pos){
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

void setFloat32To(uint8_t* pos, float flt){
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
 *  Payload functions
 */
static void Pl_setByte(Payload_t* pl, uint8_t* data, uint8_t len)
{
    uint8_t buf;

    if ( (pl->Bpos - pl->Data) > PAYLOAD_DATA_MAX_SIZE )
    {
        goto err_exit;
    }

    if ( len >  pl->bPos + 1 )
    {
        uint8_t len0 = len - 1 - pl->bPos;

        buf  = *data >> len0;
        *(pl->Bpos++) |= buf;
        if ( (pl->Bpos - pl->Data) > PAYLOAD_DATA_MAX_SIZE )
        {
            goto err_exit;
        }
        buf = *data << (8 - len0);
        *pl->Bpos |= buf;
        pl->bPos = 7 - len0;
    }
    else
    {
         buf  = *data << ( pl->bPos + 1 - len);
        *pl->Bpos |= buf;
        pl->bPos -= len;
        if ( pl->bPos == 255 )
        {
        	pl->bPos = 7;
        	pl->Bpos++;
        }
    }
    return;

err_exit:
    DLOG_MSG("Payload over flow\r\n\r\n");
    return;
}


static uint8_t* Pl_getByte(Payload_t* pl, uint8_t* val, uint8_t len)
{
   if ( len > pl->gPos + 1 )
	{
		uint8_t  len0 = len - 1 - pl->gPos;

		*val = *pl->Gpos << (7 - pl->gPos);
		*val = * val >> (8 - len);
		pl->Gpos++;
		*val |= *pl->Gpos >> (8 - len0);
		pl->gPos = 7 - len0;
	}
	else
	{
		*val = *pl->Gpos << ( 7 - pl->gPos );
		*val = *val >> (8 - len);
		pl->gPos -= len;
		if ( pl->gPos == 255 )
		{
			pl->gPos = 7;
			pl->Gpos++;
		}
	}
	return val;
}

static uint8_t* Pl_getData(Payload_t* pl, uint8_t* val, uint8_t len)
{
    if ( len < 16 )
    {
        return Pl_getByte(pl, val, len);
    }
    else if ( len < 32 )
    {
    	Pl_getByte(pl, val, 8);
    	Pl_getByte(pl, val + 1, 8);
        return val;
    }
    else
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            Pl_getByte(pl, val + i, 8);
        }
        return val;
    }
}

void ResetPayload(Payload_t* pl)
{
	memset1(pl->Data, 0, PAYLOAD_DATA_MAX_SIZE);
	pl->Bpos = pl->Data;
	pl->Gpos = pl->Data;
	pl->bPos = 7;
	pl->gPos = 7;
	pl->memDlt = 1;
	pl->Length = 0;
}

void ClearPayload(Payload_t* pl)
{
	ResetPayload(pl);
}

void SetBool(Payload_t* pl, bool flg)
{
    uint8_t data = flg ? 1 : 0;
    Pl_setByte( pl, &data, 1);
}

void SetInt4(Payload_t* pl, int8_t val)
{
    if ( val > 8 || val < -7 )
    {
        val = 0;
    }
    uint8_t data = val >= 0 && val < 8 ? val : 16 + val;
    Pl_setByte(pl, (uint8_t*)&data, 4);
}

void SetInt8(Payload_t* pl, int8_t val)
{
	SetUint8(pl, val);
}

void SetInt16(Payload_t* pl, int16_t val)
{
	SetUint16(pl, (uint16_t)val);
}

void SetInt32(Payload_t* pl, int32_t val)
{
	SetUint32(pl, val);
}

void SetFloat(Payload_t* pl, float val)
{
    uint8_t data[4];
    setFloat32To(data, val);
    for ( uint8_t i = 0;  i < 4; i++ )
    {
    	Pl_setByte(pl, data + i, 8);
    }
}

void SetUint4(Payload_t* pl, uint8_t val)
{
    uint8_t data = val;
    Pl_setByte(pl, &data, 4);
}

void SetUint8(Payload_t* pl, uint8_t val)
{
    uint8_t data = val;
    Pl_setByte(pl, &data, 8);
}

void SetUint16(Payload_t* pl, uint16_t val)
{
    uint8_t data[2];
     setUint16To(data, val);
     Pl_setByte(pl, data, 8);
     Pl_setByte(pl, data + 1, 8);
}

void SetUint24(Payload_t* pl, uint32_t val)
{
    uint8_t data[4];
     setUint32To(data, val);
     Pl_setByte(pl, data + 1, 8);
     Pl_setByte(pl, data + 2, 8);
     Pl_setByte(pl, data + 3, 8);
}

void SetUint32(Payload_t* pl, uint32_t val)
{
    uint8_t data[4];
	setUint32To(data, val);
	for ( uint8_t i = 0; i < 4; i++ )
	{
		Pl_setByte(pl, data + i, 8);
	}
}

void SetString(Payload_t* pl, char* str)
{
	uint8_t len = (uint8_t)strlen(str);
	if ( len > 15 )
	{
		len = 15;
	}
	Pl_setByte(pl, &len,4);
	for ( uint8_t i = 0; i < len; i++ )
	{
		Pl_setByte(pl, (uint8_t*)str + i, 8);
	}
}

bool GetBool(Payload_t* pl)
{
    uint8_t val;
    return (bool)*Pl_getData(pl, &val, 1);
}

int8_t GetInt4(Payload_t* pl)
{
    uint8_t val = GetUint4(pl);
    return val < 8 ? val : val - 16;
}

int8_t GetInt8(Payload_t* pl)
{
	return (int8_t) GetUint8(pl);
}

int16_t GetInt16(Payload_t* pl)
{
	return (int16_t)GetUint16(pl);
}

int32_t GetInt32(Payload_t* pl)
{
	return (int32_t)GetUint32(pl);
}

float GetFloat(Payload_t* pl)
{
    uint8_t buf[4];
    float val = getFloat32From((const uint8_t*)Pl_getData(pl, buf, 32));
    return val;
}

uint8_t GetUint4(Payload_t* pl)
{
    uint8_t val;
    Pl_getData(pl, &val, 4);
    return val;
}

uint8_t GetUint8(Payload_t* pl)
{
    uint8_t val;
   Pl_getData(pl, &val, 8);
   return val;
}

uint16_t GetUint16(Payload_t* pl)
{
    uint8_t buf[2];
    uint16_t val = getUint16From((const uint8_t*)Pl_getData(pl, buf, 16));
    return val;
}

uint32_t GetUint24(Payload_t* pl)
{
    uint8_t buf[4];
    uint8_t* p = buf + 1;
    buf[0] = 0;
	Pl_getByte(pl, p++, 8);
	Pl_getByte(pl, p++, 8);
	Pl_getByte(pl, p, 8);
    uint32_t val = getUint32From(buf);
    return val;
}

uint32_t GetUint32(Payload_t* pl)
{
    uint8_t buf[4];
    uint32_t val = getUint32From((const uint8_t*)Pl_getData(pl, buf, 32));
    return val;
}

void GetString(Payload_t* pl, char* str)
{
    uint8_t len = GetUint4(pl);
    for(uint8_t i = 0; i < len; i++)
    {
        Pl_getByte(pl, (uint8_t*)str++, 8);
    }
    *str = 0;
}

uint8_t* GetPL_RowData(Payload_t* pl)
{
	return pl->Data;
}

uint8_t GetPL_Len(Payload_t* pl)
{
    if ( pl->bPos < 7 )
    {
        return pl->Bpos - pl->Data + 1;
    }
    else
    {
        return pl->Bpos - pl->Data;
    }
}

void ReacquirePayload(Payload_t* pl)
{
    pl->Gpos = pl->Data;
    pl->gPos = 7;
}

void SetRowdataToPayload(Payload_t* pl, uint8_t* data, uint8_t length)
{
	ClearPayload(pl);
	memset1(pl->Data, 0, PAYLOAD_DATA_MAX_SIZE);
	memcpy1(pl->Data, data, length);
	pl->Length = length;
}

uint8_t GetRowdaataLength( Payload_t* pl )
{
	return pl->Length;
}
