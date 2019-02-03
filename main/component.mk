#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ArtLightApplication ?= ring

ifeq 'ring' '${ArtLightApplication}'
	CPPFLAGS += -DArtLightApplication_h=\"Ring.h\"
endif

${COMPONENT_BUILD_DIR}/%: ${COMPONENT_PATH}/%.m4; m4 -D ArtLightApplication=${ArtLightApplication} $? > $@

COMPONENT_EMBED_FILES := ${COMPONENT_PATH}/preferencesFavicon.ico ${COMPONENT_PATH}/provisionResponseFavicon ${PROJECT_PATH}/certificates/provision_ca_cert.pem ${PROJECT_PATH}/certificates/provision_ca_key.pem
COMPONENT_EMBED_TXTFILES := ${PROJECT_PATH}/certificates/ota_ca_cert.pem ${COMPONENT_BUILD_DIR}/preferences.html
