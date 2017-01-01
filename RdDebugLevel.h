// Debug level
#define RD_DBG(x, ...) Log.trace("[RD_DBG: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#define RD_INFO(x, ...) Log.info("[RD_INFO: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#define RD_WARN(x, ...) Log.warn("[RD_WARNING: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
#define RD_ERR(x, ...) Log.error("[RD_ERR: %s:%d]" x, RD_DEBUG_FNAME, __LINE__, ##__VA_ARGS__); Serial.flush();
