# Copyright (c) Martin Schweiger
# Licensed under the MIT License

set(ATM_TARGET EarthAtmNRLMSISE00)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ORBITER_BINARY_CELBODY_DIR}/${CELBODY}/Atmosphere)

add_library(${ATM_TARGET} SHARED
	EarthAtmNRLMSISE00.cpp
	nrlmsise-00.c
	nrlmsise-00_data.c
)

add_dependencies(${ATM_TARGET}
	${OrbiterTgt}
	Orbitersdk
)

target_include_directories(${ATM_TARGET}
	PUBLIC ${CMAKE_SOURCE_DIR}/Orbitersdk/include
)

target_link_libraries(${ATM_TARGET}
#	${ORBITER_LIB}
	${ORBITER_SDK_LIB}
)

set_target_properties(${ATM_TARGET}
	PROPERTIES
	FOLDER Celbody/Atmospheres
)

# Installation
install(TARGETS ${ATM_TARGET}
#	RUNTIME
	DESTINATION ${ORBITER_INSTALL_CELBODY_DIR}/${CELBODY}/Atmosphere
)
