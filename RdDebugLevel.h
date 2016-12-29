// Debug level
#ifdef RD_DEBUG_LEVEL
#if (RD_DEBUG_LEVEL > 3)
#define RD_DBG(x, ...) Serial.printlnf("[RD_DBG: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#else
#define RD_DBG(x, ...)
#endif
#else
#define RD_DBG(x, ...)
#endif

#ifdef RD_DEBUG_LEVEL
#if (RD_DEBUG_LEVEL > 2)
#define RD_INFO(x, ...) Serial.printlnf("[RD_INFO: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#else
#define RD_INFO(x, ...)
#endif
#else
#define RD_INFO(x, ...)
#endif

#ifdef RD_DEBUG_LEVEL
#if (RD_DEBUG_LEVEL > 1)
#define RD_WARN(x, ...) Serial.printlnf("[RD_WARNING: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#else
#define RD_WARN(x, ...)
#endif
#else
#define RD_WARN(x, ...)
#endif

#ifdef RD_DEBUG_LEVEL
#if (RD_DEBUG_LEVEL > 0)
#define RD_ERR(x, ...) Serial.printlnf("[RD_ERR: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#else
#define RD_ERR(x, ...)
#endif
#else
#define RD_ERR(x, ...)
#endif
