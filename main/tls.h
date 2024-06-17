#ifndef __ESP32_TLS_H__
#define __ESP32_TLS_H__

void tls_hello();

void tls_start();

int tls_next_chunk(unsigned char* buf);

void tls_terminate();

#endif
