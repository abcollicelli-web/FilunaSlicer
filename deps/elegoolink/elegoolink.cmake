# Official Apache-2.0 SDK pinned to ec2d2269. LAN only; no cloud credentials.
orcaslicer_add_cmake_project(elegoolink
    GIT_REPOSITORY https://github.com/elegooofficial/elegoo-link.git
    GIT_TAG ec2d2269b588608a1b9ccb4ff9627ddb758ea482
    DEPENDS
        dep_ixwebsocket
        dep_PahoMqttCpp
        ${CURL_PKG}
        ${OPENSSL_PKG}
    CMAKE_ARGS
        -DBUILD_EXAMPLES:BOOL=OFF
        -DBUILD_TESTS:BOOL=OFF
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DENABLE_CLOUD_FEATURES:BOOL=OFF
)