#ifndef REMOTO_H
#define REMOTO_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void Remoto_Init(void);
void Remoto_Task(uint32_t now_ms);

/* ---- Market list API ---- */
uint8_t  Remoto_MarketAdd(uint8_t product_id, uint8_t quantity);
void     Remoto_MarketClear(void);
uint8_t  Remoto_MarketGetCount(void);
uint8_t  Remoto_MarketGetItem(uint8_t index, market_item_t *out);
const char* Remoto_MarketGetProductName(uint8_t product_id);
uint8_t  Remoto_MarketGetProductCount(void);

#ifdef __cplusplus
}
#endif

#endif /* REMOTO_H */
