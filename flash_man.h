#ifndef _FLASH_MAN_H_
#define _FLASH_MAN_H_

#include <QObject>
#include <stdint.h>

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

	int Verify(uint16_t *readBuf, uint16_t *writeBuf, uint32_t len);

	void BufFree(uint16_t *buf);

signals:
	void RangeChanged(int min, int max);

	void StatusChanged(const QString &);

	void ValueChanged(int value);

};

#endif /*_FLASH_MAN_H_*/


