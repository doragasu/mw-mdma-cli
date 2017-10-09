#ifndef _FLASH_MAN_H_
#define _FLASH_MAN_H_

#include <stdint.h>

typedef enum {
	FLASHMAN_IDLE = 0,
	FLASHMAN_ERASE,
	FLASHMAN_PROGRAM,
	FLASHMAN_READ,
	FLASHMAN_DONE
} FlashState;

/// Callback for the Maximum/Minimum update
typedef void(*MaxMinCb)(int min, int max);

/// Callback for the activity update
typedef void(*ActivityCb)(FlashState st, int position);

class FlashMan {
public:
	FlashMan();

	uint16_t *Program(const char filename[], bool autoErase = false,
			uint32_t start = 0, uint32_t len = 0);

	uint16_t *Read(const char filename[], uint32_t start, uint32_t len);

	int RangeErase(uint32_t start, uint32_t len);

	int FullErase(void);

	int Verify(uint16_t *readBuf, uint16_t *writeBuf, uint32_t len);

	void SetMaxMinCallback(MaxMinCb maxMinCb);

	void SetActivityCallback(ActivityCb activityCb);

private:
	MaxMinCb maxMinCb;

	ActivityCb activityCb;

	void BufFree(uint16_t *buf);
};

#endif /*_FLASH_MAN_H_*/


