# Official Elegoo Link SDK, Apache-2.0, commit ec2d2269.
# Prefer the pinned SDK built by deps/ExternalProject (same prefix as Filuna).
# FetchContent is a fallback for a developer with the dependencies installed.
include(FetchContent)
find_package(PahoMqttCpp REQUIRED)
find_package(ixwebsocket REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(elegoolink CONFIG QUIET)

if(TARGET elegoolink::elegoolink)
    set(FILUNA_CC2_SDK_TARGET elegoolink::elegoolink)
else()
    FetchContent_Declare(filuna_elegoolink
        GIT_REPOSITORY https://github.com/elegooofficial/elegoo-link.git
        GIT_TAG ec2d2269b588608a1b9ccb4ff9627ddb758ea482
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ENABLE_CLOUD_FEATURES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(filuna_elegoolink)
    if(NOT TARGET elegoolink)
        message(FATAL_ERROR "Pinned Elegoo SDK did not define the elegoolink target")
    endif()
    set(FILUNA_CC2_SDK_TARGET elegoolink)
endif()
