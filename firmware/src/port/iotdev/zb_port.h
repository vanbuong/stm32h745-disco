#ifndef ZB_PORT_H
#define ZB_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

void zb_os_set_idle_pump(void (*fn)(void));
void zb_os_set_yield_pump(void (*fn)(void));
void zb_os_timer_pump(void);
void zb_plat_serial_poll(void);
int zb_plat_serial_ok(void);

#ifdef __cplusplus
}
#endif

#endif /* ZB_PORT_H */
