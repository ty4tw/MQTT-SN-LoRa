/**************************************************************************************
 *
 * MQTTSNTopic.c
 *
 * copyright Revised BSD License, see section \ref LICENSE
 *
 * copyright (c) 2020, Tomoaki Yamaguchi   tomoaki@tomy-tech.com
 *
 **************************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "MQTTSNTopic.h"
#include "utilities.h"

/*
 * Global variable of TopicTable
 */
MQTTSNTopicTable_t theTopicTable = { NULL };



MQTTSNTopic_t* MQTTSNTopicAdd( uint8_t* topicName, uint16_t id, uint8_t type, TopicCallback callback )
{
	MQTTSNTopic_t* last = theTopicTable.Head;
	MQTTSNTopic_t* prev = theTopicTable.Head;
	MQTTSNTopic_t* topic = NULL;

	if ( topicName == NULL )
	{
		topic = GetTopicById( id, type );
	}
	else
	{
		topic = GetTopicByName( topicName );
	}

	if ( topic == NULL )
	{
		topic = calloc( 1, sizeof(MQTTSNTopic_t) );

		if ( topic == NULL )
		{
			return NULL;
		}

		if ( last == NULL )
		{
			theTopicTable.Head = topic;
			theTopicTable.Tail = topic;
		}

		memcpy1( topic->TopicName, topicName, strlen( ( const char*)topicName ) );
		topic->TopicId = id;
		topic->TopicType = type;
		if ( callback != NULL )
		{
			topic->Callback = callback;
		}
		topic->Prev = NULL;

		while ( last )
		{
			prev = last;
			if ( prev->Next != NULL )
			{
				last = prev;
			}
			else
			{
				prev->Next = topic;
				topic->Prev = prev;
				topic->Next = NULL;
				last = 0;
				theTopicTable.Tail = topic;
			}
		}
	}
	else
	{
		if ( callback != NULL )
		{
			topic->Callback = callback;
		}
	}
	return topic;
}

void removeTopic( uint16_t topicId, uint8_t type )
{
	MQTTSNTopic_t* topic = GetTopicById( topicId, type );

	if ( topic != NULL )
	{
		if ( topic->Prev == NULL )
		{
			theTopicTable.Head = topic->Next;
			if ( topic->Next != NULL )
			{
				topic->Next->Prev = NULL;
			}
			else
			{
				theTopicTable.Tail = NULL;
			}
		}
		else
		{
			topic->Prev->Next = topic->Next;
			if ( topic->Next == NULL )
			{
				theTopicTable.Tail = topic->Prev;
			}
		}
		free( topic );
	}
}

void ClearTopicTable(void)
{
	MQTTSNTopic_t* p = theTopicTable.Head;

	while( p )
	{
		theTopicTable.Head = p->Next;

		free( p );

		p = theTopicTable.Head;
	}
	theTopicTable.Head = NULL;
	theTopicTable.Tail = NULL;
}

static uint8_t hasWildCard( uint8_t* topicName, uint8_t* pos )
{
	*pos = strlen( (const char*)topicName ) - 1;
    if ( *(topicName + *pos) == '#')
    {
        return MQTTSN_TOPIC_MULTI_WILDCARD;
    }
    else
    {
    	for(uint8_t p = 0; p < strlen( (const char*)topicName ); p++){
    		if ( *(topicName + p) == '+' )
    		{
    			*pos = p;
    			return MQTTSN_TOPIC_SINGLE_WILDCARD;
    		}
    	}
    }
    return 0;
}

bool TopicIsMatch( MQTTSNTopic_t* topic, uint8_t* topicName )
{
	uint8_t pos = 0;

	if ( strlen( (const char*)topicName ) < strlen( (const char*)topic->TopicName ) )
	{
		return false;
	}

	uint8_t wc = hasWildCard( topic->TopicName, &pos );
	if (wc == MQTTSN_TOPIC_SINGLE_WILDCARD)
	{
		if ( strncmp( (const char*)topic->TopicName, (const char*)topicName, pos - 1) == 0)
		{
			if ( *(topic->TopicName + pos + 1) == '/' )
			{
				for( uint8_t p = pos; p < strlen( (const char*)topicName); p++ )
				{
					if ( *(topicName + p) == '/' )
					{
						if (strcmp((const char*)topic->TopicName + pos + 1, (const char*)topicName + p ) == 0)
						{
							return true;
						}
					}
				}
			}
			else
			{
				for( uint8_t p = pos + 1; p < strlen( (const char*)topicName); p++ )
				{
					if ( *(topicName + p) == '/' )
					{
						return false;
					}
				}
			}
			return true;

		}
	}
	else if ( wc == MQTTSN_TOPIC_MULTI_WILDCARD )
	{
		if ( strncmp( (const char*)topic->TopicName , (const char*)topicName, pos ) == 0 )
		{
			return true;
		}
	}
	else if (strcmp( (const char*)topic->TopicName, (const char*)topicName ) == 0)
	{
		return true;

	}
	return false;
}

MQTTSNTopic_t* GetTopicMatch( uint8_t* topicName )
{
	MQTTSNTopic_t* topic = theTopicTable.Head;
	while ( topic != NULL )
	{
		if ( TopicIsMatch( topic, topicName )  )
		{
			return topic;
		}
		topic = topic->Next;
	}
	return NULL;
}

MQTTSNTopic_t*   GetTopicByName( uint8_t* topicName )
{
	MQTTSNTopic_t* topic = theTopicTable.Head;

	while ( topic != NULL )
	{
		if ( topic->TopicName != NULL && strcmp( (const char*)topic->TopicName, (const char*)topicName ) == 0  )
		{
			return topic;
		}
		topic = topic->Next;
	}
	return NULL;
}

MQTTSNTopic_t*   GetTopicById( uint16_t id, uint8_t type )
{
	MQTTSNTopic_t* topic = theTopicTable.Head;

	while ( topic )
	{
		if ( topic->TopicId == id && topic->TopicType == type )
		{
			return topic;
		}
		topic = topic->Next;
	}
	return NULL;
}

void SetTopicId( uint8_t* topicName, uint16_t id, uint8_t topicType )
{
	MQTTSNTopic_t* topic = GetTopicByName( topicName );

    if ( topic )
    {
    	topic->TopicId = id;
    }
    else
    {
    	MQTTSNTopicAdd( topicName, id, topicType, NULL );
    }
}

void MQTTSNTopicExecCallback( uint16_t  topicId, uint8_t topicType, Payload_t* payload )
{
	MQTTSNTopic_t* topic = GetTopicById( topicId, topicType );

	if ( topic != NULL  && topic->Callback != NULL )
	{
		topic->Callback( payload );
	}
}
