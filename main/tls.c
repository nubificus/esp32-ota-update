#include "tls.h"
#include <stdio.h>

#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_server_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_server_crt_end");
extern const uint8_t server_key_pem_start[] asm("_binary_server_key_start");
extern const uint8_t server_key_pem_end[] asm("_binary_server_key_end");

mbedtls_net_context listen_fd, client_fd;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_x509_crt srvcert;
mbedtls_pk_context pkey;

void tls_start () {
  
  static const char *TAG = "tls_server";
  int ret;

  mbedtls_net_init(&listen_fd);
  mbedtls_net_init(&client_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&srvcert);
  mbedtls_pk_init(&pkey);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  
  const char *pers = "ssl_server";
  
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
    ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
    abort();
  }

  // Load the server certificate and key
  ret = mbedtls_x509_crt_parse(&srvcert, server_cert_pem_start, server_cert_pem_end - server_cert_pem_start);
  if (ret != 0) {
    ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned %d", ret);
    abort();
  }

  ret = mbedtls_pk_parse_key(&pkey, server_key_pem_start, server_key_pem_end - server_key_pem_start, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg);
  if (ret != 0) {
    ESP_LOGE(TAG, "mbedtls_pk_parse_key returned %d", ret);
    abort();
  }

  if ((ret = mbedtls_net_bind(&listen_fd, NULL, "4433", MBEDTLS_NET_PROTO_TCP)) != 0) {
    ESP_LOGE(TAG, "mbedtls_net_bind returned %d", ret);
    abort();
  }

  if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
    abort();
  }

  mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);
  if ((ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_conf_own_cert returned %d", ret);
    abort();
  }
 
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

  if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
     ESP_LOGE(TAG, "mbedtls_ssl_setup returned %d", ret);
     abort();
  }

  ESP_LOGI(TAG, "Waiting for a connection...");

  if ((ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL)) != 0) {
    ESP_LOGE(TAG, "mbedtls_net_accept returned %d", ret);
    abort();
  }

  ESP_LOGI(TAG, "Connection accepted.");

  mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  if ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
    abort();
  }

  ESP_LOGI(TAG, "Handshake successful.");
}


int tls_next_chunk(unsigned char* buf) {
    return mbedtls_ssl_read(&ssl, buf, sizeof(buf));
}

void tls_terminate() {
    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&client_fd);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&srvcert);
    mbedtls_pk_free(&pkey);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

void tls_hello() {
  printf("Server Key:\n%s\n\n\nServer Cert:\n%s\n\n", server_key_pem_start, server_cert_pem_start);
}
