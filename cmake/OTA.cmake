set(OTA ${CONAN_OTA_ROOT})
include(PPC)

file(GLOB SOURCE_FILES ${OTA}/src/*)

add_library(OTA STATIC ${SOURCE_FILES})
target_include_directories(OTA PRIVATE ${OTA}/include ${MQTT}/include ${PLATFORM_CXX_INCLUDES})
target_compile_options(OTA PRIVATE "$<$<CONFIG:ALL>:${PLATFORM_CXX_FLAGS}>")
target_compile_definitions(OTA PRIVATE ${PLATFORM_CXX_DEFS})
add_dependencies(OTA PPC)
