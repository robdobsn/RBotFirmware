#pragma once

#include <stdlib.h>

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
    JSMNR_UNDEFINED = 0,
    JSMNR_OBJECT = 1,
    JSMNR_ARRAY = 2,
    JSMNR_STRING = 3,
    JSMNR_PRIMITIVE = 4
} jsmnrtype_t;

typedef enum {
    /* Not enough tokens were provided */
    JSMNR_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
    JSMNR_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMNR_ERROR_PART = -3,
    /* Everything was fine */
    JSMNR_SUCCESS = 0
} jsmnrerr_t;

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct jsmnrtok_t {
    jsmnrtype_t type;
    int start;
    int end;
    int size;
#ifdef JSMNR_PARENT_LINKS
    int parent;
#endif
} jsmnrtok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
    unsigned int pos; /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper; /* superior token node, e.g parent object or array */
} JSMNR_parser;

/**
 * Create JSON parser over an array of tokens
 */
void JSMNR_init(JSMNR_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 * Returns count of parsed objects.
 */
int JSMNR_parse(JSMNR_parser *parser, const char *js, size_t len,
                jsmnrtok_t *tokens, unsigned int num_tokens);


void JSMNR_logLongStr(const char* headerMsg, const char* toLog, bool infoLevel = false);
