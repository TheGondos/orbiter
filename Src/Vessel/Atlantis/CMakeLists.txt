# Copyright (c) Martin Schweiger
# Licensed under the MIT License

add_subdirectory(Atlantis)
add_subdirectory(Atlantis_SRB)
add_subdirectory(Atlantis_Tank)
#add_subdirectory(AtlantisConfig)
#add_subdirectory(Help)

set(BITMAP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Bitmaps)
install(DIRECTORY ${BITMAP_SOURCE_DIR}/
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}/Bitmaps/Atlantis
)
