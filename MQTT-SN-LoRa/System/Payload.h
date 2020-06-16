/*!
 * \file      Payload.h
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#ifndef APPCTRL_PAYLOAD_H_
#define APPCTRL_PAYLOAD_H_

#include <stdio.h>
#include <stdbool.h>

#define PAYLOAD_DATA_MAX_SIZE           242

/*!
 *  Payload
 */
typedef struct
{
	uint8_t  Data[PAYLOAD_DATA_MAX_SIZE];
	uint8_t  Length;
	uint8_t* Bpos;
	uint8_t  bPos;
	uint8_t  gPos;
	uint8_t* Gpos;
	uint8_t  memDlt;
}Payload_t;


/*!
 *  \brief Payload's getters
 *
 *  \param [IN] pl       Payload
 *
 *  \retval data         value
 *
 */
bool     GetBool(Payload_t* pl);
int8_t   GetInt4(Payload_t* pl);
int8_t   GetInt8(Payload_t* pl);
int16_t  GetInt16(Payload_t* pl);
int32_t  GetInt32(Payload_t* pl);
float   GetFloat(Payload_t* pl);
uint8_t  GetUint4(Payload_t* pl);
uint8_t  GetUint8(Payload_t* pl);
uint16_t GetUint16(Payload_t* pl);
uint32_t GetUint24(Payload_t* pl);
uint32_t GetUint32(Payload_t* pl);
void    GetString(Payload_t* pl, char* str);


/*!
 *  \brief Payload's setters
 *
 *  \param [IN] pl       Payload
 *  \param [IN] val      Value to set
 *
 */
void SetBool(Payload_t* pl, bool val);
void SetInt4(Payload_t* pl, int8_t val);
void SetInt8(Payload_t* pl, int8_t val);
void SetInt16(Payload_t* pl, int16_t val);
void SetInt32(Payload_t* pl, int32_t val);
void SetFloat(Payload_t* pl, float val);
void SetUint4(Payload_t* pl, uint8_t val);
void SetUint8(Payload_t* pl, uint8_t val);
void SetUint16(Payload_t* pl, uint16_t val);
void SetUint24(Payload_t* pl, uint32_t val);
void SetUint32(Payload_t* pl, uint32_t val);
void SetString(Payload_t* pl, char* val);


/**
 *  Big Endian convert
 */
uint16_t getUint16From(const uint8_t* pos);
uint32_t getUint32From(const uint8_t* pos);
float    getFloat32From(const uint8_t* pos);
void setUint16To(uint8_t* pos, uint16_t val);
void setUint32To(uint8_t* pos, uint32_t val);
void setFloat32To(uint8_t* pos, float flt);



/*!
 *  \brief Get Rowdata
 *
 *  \param [IN] pl       Payload
 *  \retval data         Row data
 *
 */
uint8_t* GetPL_RowData(Payload_t* pl);

/*!
 *  \brief Get a length of the payload
 *
 *  \param [IN] pl       Payload
 *  \retval data         length of the payload
 *
 */
uint8_t  GetPL_Len(Payload_t* pl);

/*!
 *  \brief Clear values of the payload
 *
 *  \param [IN] pl       Payload
 *
 */
void    ClearPayload(Payload_t* pl);

/*!
 *  \brief Reset values of the payload same as ClearPayload
 *
 *  \param [IN] pl       Payload
 *
 */
void    ResetPayload(Payload_t* pl);

/*!
 *  \brief Return to the first variable of Payload.
 *
 *  \param [IN] pl       Payload
 *
 */
void    ReacquirePayload(Payload_t* pl);

/*!
 *  \brief Set Rowdata into Payload.
 *
 *  \param [IN] pl         Payload
 *  \param [IN] data       Rowdata
 *  \param [IN] length     Size of Rowdata
 *
 */
void SetRowdataToPayload(Payload_t* pl, uint8_t* data, uint8_t length);

uint8_t GetRowdaataLength( Payload_t* pl );

#endif /* APPCTRL_PAYLOAD_H_ */
