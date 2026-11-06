# Filuna CC2 SDK integration (Elegoo Link, Apache-2.0).
# Keep isolated from the existing Orca/ElegooLink HTTP implementation until
# the complete SDK-based device flow has passed a Windows build and UI test.
include(FetchContent)
find_package(PahoMqttCpp REQUIRED)
find_package(ixwebsocket REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

FetchContent_Declare(filuna_elegoolink
  GIT_REPOSITORY https://github.com/elegooofficial/elegoo-link.git
  GIT_TAG ec2d2269b588608a1b9ccb4ff9627ddb758ea482
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
# This SDK is linked as a static LAN-only library. It has no Agora/cloud dependency.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_CLOUD_FEATURES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(filuna_elegoolink)
if(NOT TARGET elegoolink)
  message(FATAL_ERROR "Elegoo SDK ec2d2269 did not define the elegoolink target")
endif()
