/*
 * LoRaLinkCrypto.c
 *
 *  Created on: 2020/06/14
 *      Author: tomoaki
 */
#include "LoRaLinkTypes.h"
#include "LoRaLink.h"
#include "LoRaLinkCrypto.h"
#include "aes.h"
#include "cmac.h"
#include "utilities.h"

/*
 * CMAC/AES Message Integrity Code (MIC) Block B0 size
 */
#define MIC_BLOCK_BX_SIZE               16
/*
 * Maximum size of the message that can be handled by the crypto operations
 */
#define CRYPTO_MAXMESSAGE_SIZE          256
/*
 * Maximum size of the buffer for crypto operations
 */
#define CRYPTO_BUFFER_SIZE              CRYPTO_MAXMESSAGE_SIZE + MIC_BLOCK_BX_SIZE

/*
 * Secure Element Non Volatile Context structure
 */
typedef struct sSecureElementNvCtx
{
    /*
     * AES computation context variable
     */
    aes_context AesContext;
    /*
     * CMAC computation context variable
     */
    AES_CMAC_CTX AesCmacCtx[1];
    /*
     * Key
     */
    Key_t Key;
}SecureElementNvCtx_t;

SecureElementNvCtx_t SeCtx;

static LoRaLinkCryptoStatus_t PrepareB0( uint16_t msgLen, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint8_t* b0 );

static SecureElementStatus_t SecureElementAesEncrypt( uint8_t* buffer, uint16_t size, Key_t* key, uint8_t* encBuffer );
static LoRaLinkCryptoStatus_t ComputeCmacB0( uint8_t* msg, uint16_t len, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint32_t* cmac );
static LoRaLinkCryptoStatus_t VerifyCmacB0( uint8_t* msg, uint16_t len, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint32_t cmac );
static LoRaLinkCryptoStatus_t PayloadEncrypt( uint8_t* buffer, int16_t size, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr );
static SecureElementStatus_t SecureElementComputeAesCmac( uint8_t *micBxBuffer, uint8_t *buffer, uint16_t size, Key_t* key, uint32_t* cmac );
static SecureElementStatus_t SecureElementVerifyAesCmac( uint8_t* buffer, uint16_t size, uint32_t expectedCmac, Key_t* keyID );

void LoRaLinkCryptoSetKey(uint8_t* key)
{
	memcpy1(SeCtx.Key.KeyValue, key, MIC_BLOCK_BX_SIZE);
}

LoRaLinkCryptoStatus_t LoRaLinkCryptoSecureMessage(LoRaLinkPacket_t* packet)
{
    if( packet == NULL )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }

    if ( PayloadEncrypt( packet->FRMPayload, packet->FRMPayloadSize, &SeCtx.Key, packet->PanId, packet->DestAddr, packet->SourceAddr ) != LORALINK_CRYPTO_SUCCESS )
	{
		return LORALINK_CRYPTO_ERROR;
	}

    // Serialize message
    if( LoRaLinkSerializeData( packet ) != LORALINK_STATUS_OK )
    {
        return LORALINK_CRYPTO_ERROR_SERIALIZER;
    }

    LoRaLinkCryptoStatus_t retval = ComputeCmacB0( packet->Buffer, ( packet->BufSize - LORALINK_MIC_FIELD_SIZE ), &SeCtx.Key, packet->PanId, packet->DestAddr, packet->SourceAddr, &packet->MIC );

    if ( retval != LORALINK_CRYPTO_SUCCESS )
    {
    	return retval;
    }

    // Re-serialize message to add the MIC
	if( LoRaLinkSerializeData( packet ) != LORALINK_STATUS_OK )
	{
		return LORALINK_CRYPTO_ERROR_SERIALIZER;
	}

	return LORALINK_CRYPTO_SUCCESS;
}

LoRaLinkCryptoStatus_t LoRaLinkCryptoUnsecureMessage( LoRaLinkPacket_t* packet )
{
    if( packet == NULL )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }

    // Verify mic
    if ( VerifyCmacB0( packet->Buffer, ( packet->BufSize - LORALINK_MIC_FIELD_SIZE ),  &SeCtx.Key, packet->PanId, packet->DestAddr, packet->SourceAddr, packet->MIC ) != LORALINK_CRYPTO_SUCCESS )
    {
        return LORALINK_CRYPTO_ERROR;
    }

    return PayloadEncrypt( packet->FRMPayload, packet->FRMPayloadSize, &SeCtx.Key, packet->PanId, packet->DestAddr, packet->SourceAddr );
}

static LoRaLinkCryptoStatus_t PayloadEncrypt( uint8_t* buffer, int16_t size, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr )
{
    if( buffer == NULL )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }

    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;
    uint8_t sBlock[16] = { 0 };
    uint8_t aBlock[16] = { 0 };

    aBlock[0] = 0x01;

    aBlock[5] = panId & 0xFF;
    aBlock[6] = ( panId >> 8 ) & 0xFF;

    aBlock[9] = destAddr;

    aBlock[13] = srcAddr;

    while( size > 0 )
    {
        aBlock[15] = ctr & 0xFF;
        ctr++;
        if( SecureElementAesEncrypt( aBlock, 16, key, sBlock ) != SECURE_ELEMENT_SUCCESS )
        {
            return LORALINK_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
        }

        for( uint8_t i = 0; i < ( ( size > 16 ) ? 16 : size ); i++ )
        {
            buffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
        }
        size -= 16;
        bufferIndex += 16;
    }

    return LORALINK_CRYPTO_SUCCESS;
}

static SecureElementStatus_t SecureElementAesEncrypt( uint8_t* buffer, uint16_t size, Key_t* key, uint8_t* encBuffer )
{
    if( buffer == NULL || encBuffer == NULL )
    {
        return SECURE_ELEMENT_ERROR_NPE;
    }

    // Check if the size is divisible by 16,
    if( ( size % 16 ) != 0 )
    {
        return SECURE_ELEMENT_ERROR_BUF_SIZE;
    }

    memset1( SeCtx.AesContext.ksch, '\0', 240 );

    aes_set_key( key->KeyValue, 16, &SeCtx.AesContext );

    uint8_t block = 0;

    while( size != 0 )
    {
        aes_encrypt( &buffer[block], &encBuffer[block], &SeCtx.AesContext );
        block = block + 16;
        size = size - 16;
    }
    return SECURE_ELEMENT_SUCCESS;
}


static LoRaLinkCryptoStatus_t ComputeCmacB0( uint8_t* msg, uint16_t len, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint32_t* cmac )
{
    if( ( msg == NULL ) || ( cmac == NULL ) )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }
    if( len > CRYPTO_MAXMESSAGE_SIZE )
    {
        return LORALINK_CRYPTO_ERROR_BUF_SIZE;
    }

    uint8_t micBuff[MIC_BLOCK_BX_SIZE];

    // Initialize the first Block
    PrepareB0( len, panId, destAddr, srcAddr, micBuff );

    if( SecureElementComputeAesCmac( micBuff, msg, len, key, cmac ) != SECURE_ELEMENT_SUCCESS )
	{
		return LORALINK_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
	}
    return LORALINK_CRYPTO_SUCCESS;
}

static LoRaLinkCryptoStatus_t VerifyCmacB0( uint8_t* msg, uint16_t len, Key_t* key, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint32_t expectedCmac )
{
    if( msg == NULL )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }
    if( len > CRYPTO_MAXMESSAGE_SIZE )
    {
        return LORALINK_CRYPTO_ERROR_BUF_SIZE;
    }

    uint8_t micBuff[CRYPTO_BUFFER_SIZE];
    memset1( micBuff, 0, CRYPTO_BUFFER_SIZE );

    // Initialize the first Block
    PrepareB0( len, panId, destAddr, srcAddr, micBuff );

    // Copy the given data to the mic computation buffer
    memcpy1( ( micBuff + MIC_BLOCK_BX_SIZE ), msg, len );

    SecureElementStatus_t retval = SECURE_ELEMENT_ERROR;
    retval = SecureElementVerifyAesCmac( micBuff, ( len + MIC_BLOCK_BX_SIZE ), expectedCmac, &SeCtx.Key );

    if( retval == SECURE_ELEMENT_SUCCESS )
    {
        return LORALINK_CRYPTO_SUCCESS;
    }
    else if( retval == SECURE_ELEMENT_FAIL_CMAC )
    {
        return LORALINK_CRYPTO_FAIL_MIC;
    }

    return LORALINK_CRYPTO_ERROR_SECURE_ELEMENT_FUNC;
}

static LoRaLinkCryptoStatus_t PrepareB0( uint16_t msgLen, uint16_t panId, uint8_t destAddr, uint8_t srcAddr, uint8_t* b0 )
{
    if( b0 == NULL )
    {
        return LORALINK_CRYPTO_ERROR_NPE;
    }

    b0[0] = 0x49;

    b0[1] = 0x00;
    b0[2] = 0x00;
    b0[3] = 0x00;
    b0[4] = 0x00;

    b0[5] = msgLen & 0xFF;

    b0[6] = panId & 0xFF;
    b0[7] = ( panId >> 8 ) & 0xFF;
    b0[8] = 0x00;
    b0[9] = 0x00;

    b0[10] = destAddr;
    b0[11] = 0x00;
    b0[12] = 0x00;
    b0[13] = 0x00;

    b0[14] = srcAddr;

    b0[15] = 0x0F;

    return LORALINK_CRYPTO_SUCCESS;
}



/*
 * Computes a CMAC of a message using provided initial Bx block
 *
 *  cmac = aes128_cmac(keyID, blocks[i].Buffer)
 *
 * \param[IN]  micBxBuffer    - Buffer containing the initial Bx block
 * \param[IN]  buffer         - Data buffer
 * \param[IN]  size           - Data buffer size
 * \param[IN]  keyID          - Key identifier to determine the AES key to be used
 * \param[OUT] cmac           - Computed cmac
 * \retval                    - Status of the operation
 */
static SecureElementStatus_t ComputeCmac( uint8_t *micBxBuffer, uint8_t *buffer, uint16_t size, Key_t* key, uint32_t* cmac )
{
    if( ( buffer == NULL ) || ( cmac == NULL ) )
    {
        return SECURE_ELEMENT_ERROR_NPE;
    }

    uint8_t Cmac[16];

    AES_CMAC_Init( SeCtx.AesCmacCtx );

	AES_CMAC_SetKey( SeCtx.AesCmacCtx, key->KeyValue );

	if( micBxBuffer != 0 )
	{
		AES_CMAC_Update( SeCtx.AesCmacCtx, micBxBuffer, 16 );
	}

	AES_CMAC_Update( SeCtx.AesCmacCtx, buffer, size );

	AES_CMAC_Final( Cmac, SeCtx.AesCmacCtx );

	// Bring into the required format
	*cmac = ( uint32_t )( ( uint32_t ) Cmac[3] << 24 | ( uint32_t ) Cmac[2] << 16 | ( uint32_t ) Cmac[1] << 8 | ( uint32_t ) Cmac[0] );

    return SECURE_ELEMENT_SUCCESS;
}


static SecureElementStatus_t SecureElementComputeAesCmac( uint8_t *micBxBuffer, uint8_t *buffer, uint16_t size, Key_t* key, uint32_t* cmac )
{
    return ComputeCmac( micBxBuffer, buffer, size, key, cmac );
}

static SecureElementStatus_t SecureElementVerifyAesCmac( uint8_t* buffer, uint16_t size, uint32_t expectedCmac, Key_t* keyID )
{
    if( buffer == NULL )
    {
        return SECURE_ELEMENT_ERROR_NPE;
    }

    SecureElementStatus_t retval = SECURE_ELEMENT_ERROR;
    uint32_t compCmac = 0;
    retval = ComputeCmac( NULL, buffer, size, keyID, &compCmac );
    if( retval != SECURE_ELEMENT_SUCCESS )
    {
        return retval;
    }

    if( expectedCmac != compCmac )
    {
        retval = SECURE_ELEMENT_FAIL_CMAC;
    }

    return retval;
}
