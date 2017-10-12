#ifndef _FLASH_MAN_H_
#define _FLASH_MAN_H_

#include <QObject>
#include <stdint.h>

/// Chip length in bytes
#define FM_CHIP_LENGTH	0x400000

typedef enum {
	FLASHMAN_IDLE = 0,
	FLASHMAN_ERASE,
	FLASHMAN_PROGRAM,
	FLASHMAN_READ,
	FLASHMAN_DONE
} FlashState;

class FlashMan : public QObject {
	Q_OBJECT
public:
	uint16_t *Program(const char filename[], bool autoErase,
			uint32_t *start, uint32_t *len);

	uint16_t *Read(uint32_t start, uint32_t len);

	int RangeErase(uint32_t start, uint32_t len);

	int FullErase(void);

	void BufFree(uint16_t *buf);

	uint16_t ManIdGet(uint16_t *manId);

	uint16_t DevIdGet(uint16_t devIds[3]);

signals:
	void RangeChanged(int min, int max);

	void StatusChanged(const QString &status);

	void ValueChanged(int value);

};

#endif /*_FLASH_MAN_H_*/


