idf_component_register(
	SRCS 
	  "main.c"
	  "tls.c"
	  "wifi.c"
	  "ota.c"
	INCLUDE_DIRS "." 
	REQUIRES esp_partition app_update esp_netif esp_wifi nvs_flash mbedtls
	EMBED_TXTFILES cert/server.crt cert/server.key
)
