/*
 * LoRaLinkCrypt.h
 *
 *  Created on: 2020/06/14
 *      Author: tomoaki
 */

#ifndef LORALINKCRYPTO_H_
#define LORALINKCRYPTO_H_

#include <stdint.h>

#define KEY_SIZE         16

/*!
 * LoRaMac Cryto Status
 */
typedef enum
{
    /*!
     * No error occurred
     */
    LORALINK_CRYPTO_SUCCESS = 0,
    /*!
     * MIC does not match
     */
	LORALINK_CRYPTO_FAIL_MIC,
    /*!
     * Address does not match
     */
	LORALINK_CRYPTO_FAIL_ADDRESS,
    /*!
     * Not allowed parameter value
     */
	LORALINK_CRYPTO_FAIL_PARAM,
    /*!
     * Null pointer exception
     */
	LORALINK_CRYPTO_ERROR_NPE,
    /*!
     * Invalid address identifier exception
     */
    LORALINK_CRYPTO_ERROR_INVALID_ADDR_ID,
    /*!
     * Incompatible buffer size
     */
    LORALINK_CRYPTO_ERROR_BUF_SIZE,
    /*!
     * The secure element reports an error
     */
    LORALINK_CRYPTO_ERROR_SECURE_ELEMENT_FUNC,
    /*!
     * Error from parser reported
     */
    LORALINK_CRYPTO_ERROR_PARSER,
    /*!
     * Error from serializer reported
     */
    LORALINK_CRYPTO_ERROR_SERIALIZER,
    /*!
     * RJcount1 reached 2^16-1 which should never happen
     */
    LORALINK_CRYPTO_ERROR_RJCOUNT1_OVERFLOW,
    /*!
     * Undefined Error occurred
     */
    LORALINK_CRYPTO_ERROR,
}LoRaLinkCryptoStatus_t;

#define KEY_SIZE         16

/*!
 * Return values.
 */
typedef enum eSecureElementStatus
{
    /*!
     * No error occurred
     */
    SECURE_ELEMENT_SUCCESS = 0,
    /*!
     * CMAC does not match
     */
    SECURE_ELEMENT_FAIL_CMAC,
    /*!
     * Null pointer exception
     */
    SECURE_ELEMENT_ERROR_NPE,
    /*!
     * Invalid key identifier exception
     */
    SECURE_ELEMENT_ERROR_INVALID_KEY_ID,
    /*!
     * Invalid LoRaWAN specification version
     */
    SECURE_ELEMENT_ERROR_INVALID_LORAWAM_SPEC_VERSION,
    /*!
     * Incompatible buffer size
     */
    SECURE_ELEMENT_ERROR_BUF_SIZE,
    /*!
     * Undefined Error occurred
     */
    SECURE_ELEMENT_ERROR,
}SecureElementStatus_t;

/*!
 * Key value
 */
typedef struct
{
    uint8_t KeyValue[KEY_SIZE];
} Key_t;

LoRaLinkCryptoStatus_t LoRaLinkCryptoSecureMessage(LoRaLinkPacket_t* packet);
LoRaLinkCryptoStatus_t LoRaLinkCryptoUnsecureMessage( LoRaLinkPacket_t* packet );
void LoRaLinkCryptoSetKey(uint8_t* key);

#endif /* LORALINKCRYPTO_H_ */
