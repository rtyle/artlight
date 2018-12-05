#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_EMBED_FILES := ${PROJECT_PATH}/main/provisionResponseFavicon ${PROJECT_PATH}/certificates/provision_ca_cert.pem ${PROJECT_PATH}/certificates/provision_ca_key.pem
COMPONENT_EMBED_TXTFILES := ${PROJECT_PATH}/certificates/firmware_ca_cert.pem ${PROJECT_PATH}/main/preferences.html
