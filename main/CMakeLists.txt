set(COMPONENT_SRCS "simple_ota_example.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")


# Embed the server root certificate into the final binary
set(COMPONENT_EMBED_FILES ${PROJECT_PATH}/main/provisionResponseFavicon ${PROJECT_PATH}/certificates/provision_ca_cert.pem ${PROJECT_PATH}/certificates/provision_ca_key.pem)
set(COMPONENT_EMBED_TXTFILES ${PROJECT_PATH}/certificates/ota_ca_cert.pem ${PROJECT_PATH}/main/preferences.html

register_component()
