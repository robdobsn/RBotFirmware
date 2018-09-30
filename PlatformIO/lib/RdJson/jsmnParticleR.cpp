// Adapted from jsmnSpark at https://github.com/pkourany/JSMNSpark
// Merged changes from original https://github.com/zserge/jsmn
// Rob Dobson 2017-2018

#include <ArduinoLog.h>
#include "jsmnParticleR.h"

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmnrtok_t *JSMNR_alloc_token(JSMNR_parser *parser,
		jsmnrtok_t *tokens, size_t num_tokens) {
	jsmnrtok_t *tok;
	if (parser->toknext >= num_tokens) {
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
#ifdef JSMNR_PARENT_LINKS
	tok->parent = -1;
#endif
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void JSMNR_fill_token(jsmnrtok_t *token, jsmnrtype_t type,
                            int start, int end) {
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int JSMNR_parse_primitive(JSMNR_parser *parser, const char *js,
		size_t len, jsmnrtok_t *tokens, size_t num_tokens) {
	jsmnrtok_t *token;
	int start;

	start = parser->pos;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		switch (js[parser->pos]) {
#ifndef JSMNR_STRICT
			/* In strict mode primitive must be followed by "," or "}" or "]" */
			case ':':
#endif
			case '\t' : case '\r' : case '\n' : case ' ' :
			case ','  : case ']'  : case '}' :
				goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
            Log.trace("JSMNR_ERROR_INVAL ch bounds %d pos %d\n", js[parser->pos], parser->pos);
            parser->pos = start;
			return JSMNR_ERROR_INVAL;
		}
	}
#ifdef JSMNR_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMNR_ERROR_PART;
#endif

found:
	if (tokens == NULL) {
		parser->pos--;
		return 0;
	}
	token = JSMNR_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMNR_ERROR_NOMEM;
	}
	JSMNR_fill_token(token, JSMNR_PRIMITIVE, start, parser->pos);
#ifdef JSMNR_PARENT_LINKS
	token->parent = parser->toksuper;
#endif
	parser->pos--;
	return 0;
}

/**
 * Fills next token with JSON string.
 */
static int JSMNR_parse_string(JSMNR_parser *parser, const char *js,
		size_t len, jsmnrtok_t *tokens, size_t num_tokens) {
	jsmnrtok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c = js[parser->pos];

		// Quote: end of string
		if (c == '\"') {
			if (tokens == NULL) {
				return 0;
			}
			token = JSMNR_alloc_token(parser, tokens, num_tokens);
			if (token == NULL) {
				parser->pos = start;
				return JSMNR_ERROR_NOMEM;
			}
			JSMNR_fill_token(token, JSMNR_STRING, start+1, parser->pos);
#ifdef JSMNR_PARENT_LINKS
			token->parent = parser->toksuper;
#endif
			return JSMNR_SUCCESS;
		}

		// Backslash: Quoted symbol expected
		if (c == '\\' && parser->pos + 1 < len) {
			parser->pos++;
			switch (js[parser->pos]) {
				// Allowed escaped symbols
				case '\"': case '/' : case '\\' : case 'b' :
				case 'f' : case 'r' : case 'n'  : case 't' :
					break;
				// Allows escaped symbol \uXXXX
				case 'u':
					parser->pos++;
					for(int i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
						// If it isn't a hex character we have an error
						if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || // 0-9
									(js[parser->pos] >= 65 && js[parser->pos] <= 70) || // A-F
									(js[parser->pos] >= 97 && js[parser->pos] <= 102))) { // a-f
                            Log.trace("JSMNR_ERROR_INVAL hex bounds %d pos %d\n", js[parser->pos], parser->pos);
							parser->pos = start;
							return JSMNR_ERROR_INVAL;
						}
						parser->pos++;
					}
					parser->pos--;
					break;
				// Unexpected symbol
				default:
                    Log.trace("JSMNR_ERROR_INVAL Unexpected %d pos %d\n", js[parser->pos], parser->pos);
					parser->pos = start;
					return JSMNR_ERROR_INVAL;
			}
		}

	}
	parser->pos = start;
	return JSMNR_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int JSMNR_parse(JSMNR_parser *parser, const char *js, size_t len,
		jsmnrtok_t *tokens, unsigned int num_tokens) {
	int r;
	int i;
	jsmnrtok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmnrtype_t type;

		c = js[parser->pos];
		switch (c) {
			case '{':
            case '[':
				count++;
				if (tokens == NULL) {
					break;
				}
				token = JSMNR_alloc_token(parser, tokens, num_tokens);
				if (token == NULL)
					return JSMNR_ERROR_NOMEM;
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
#ifdef JSMNR_PARENT_LINKS
					token->parent = parser->toksuper;
#endif
				}
				token->type = (c == '{' ? JSMNR_OBJECT : JSMNR_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;
				break;
			case '}':
            case ']':
				if (tokens == NULL)
					break;
				type = (c == '}' ? JSMNR_OBJECT : JSMNR_ARRAY);
#ifdef JSMNR_PARENT_LINKS
				if (parser->toknext < 1) {
					return JSMNR_ERROR_INVAL;
				}
				token = &tokens[parser->toknext - 1];
				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMNR_ERROR_INVAL;
						}
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1) {
						if(token->type != type || parser->toksuper == -1) {
							return JSMNR_ERROR_INVAL;
						}
						break;
					}
					token = &tokens[token->parent];
				}
#else
				for (i = parser->toknext - 1; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
                            Log.notice("JSMNR_ERROR_INVAL %d type %d %d\n", token->start, token->type, type);
							return JSMNR_ERROR_INVAL;
						}
						parser->toksuper = -1;
						token->end = parser->pos + 1;
						break;
					}
				}
				/* Error if unmatched closing bracket */
				if (i == -1)
                {
                    Log.notice("JSMNR_ERROR_INVAL unmatchedbrace pos %d ch %d toknext %d type %d\n", parser->pos, js[parser->pos], parser->toknext, type);
                    JSMNR_logLongStr("JSMN: parse input", js, true);
                    return JSMNR_ERROR_INVAL;
                }
				for (; i >= 0; i--) {
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {
						parser->toksuper = i;
						break;
					}
				}
#endif
				break;
			case '\"':
				r = JSMNR_parse_string(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;
			case '\t' : case '\r' : case '\n' : case ' ':
				break;
			case ':':
				parser->toksuper = parser->toknext - 1;
				break;
			case ',':
				if (tokens != NULL && parser->toksuper != -1 &&
						tokens[parser->toksuper].type != JSMNR_ARRAY &&
						tokens[parser->toksuper].type != JSMNR_OBJECT) {
#ifdef JSMNR_PARENT_LINKS
					parser->toksuper = tokens[parser->toksuper].parent;
#else
					for (i = parser->toknext - 1; i >= 0; i--) {
						if (tokens[i].type == JSMNR_ARRAY || tokens[i].type == JSMNR_OBJECT) {
							if (tokens[i].start != -1 && tokens[i].end == -1) {
								parser->toksuper = i;
								break;
							}
						}
					}
#endif
				}
				break;
#ifdef JSMNR_STRICT
			/* In strict mode primitives are: numbers and booleans */
			case '-': case '0': case '1' : case '2': case '3' : case '4':
			case '5': case '6': case '7' : case '8': case '9':
			case 't': case 'f': case 'n' :
				/* And they must not be keys of the object */
				if (tokens != NULL && parser->toksuper != -1) {
					jsmnrtok_t *t = &tokens[parser->toksuper];
					if (t->type == JSMNR_OBJECT ||
							(t->type == JSMNR_STRING && t->size != 0)) {
						return JSMNR_ERROR_INVAL;
					}
				}
#else
			/* In non-strict mode every unquoted value is a primitive */
			default:
#endif
				r = JSMNR_parse_primitive(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;

#ifdef JSMNR_STRICT
			/* Unexpected char in strict mode */
			default:
				return JSMNR_ERROR_INVAL;
#endif
		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMNR_ERROR_PART;
			}
		}
	}

	return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void JSMNR_init(JSMNR_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

// Helper function to log long strings
void JSMNR_logLongStr(const char* headerMsg, const char* toLog, bool infoLevel)
{
    if (infoLevel)
        Log.notice(headerMsg);
    else
        Log.trace(headerMsg);
    const int linLen = 80;
    for (unsigned int i = 0; i < strlen(toLog); i+=linLen)
    {
        char pBuf[linLen+1];
        strncpy(pBuf, toLog+i, linLen);
        pBuf[linLen] = 0;
        if (infoLevel)
            Log.notice(pBuf);
        else
            Log.trace(pBuf);
    }
}
