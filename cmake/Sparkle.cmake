# Sparkle (https://sparkle-project.org), the updater behind "Check for
# Updates…": the release archive is fetched once per build tree, checked
# against its SHA-256, and Sparkle.framework is embedded in metronom.app.
#
# Sets SPARKLE_FRAMEWORK (the framework to embed) and SPARKLE_BIN_DIR
# (sign_update and generate_keys, for the release workflow).

set(METRONOM_SPARKLE_VERSION 2.10.1)
set(METRONOM_SPARKLE_SHA256 1a856d2a9edb8e628de88121d7be04cf0e2167f8641650b78fca00b14244cf27)

set(sparkle_root "${CMAKE_BINARY_DIR}/_deps/sparkle-${METRONOM_SPARKLE_VERSION}")
set(sparkle_archive "${CMAKE_BINARY_DIR}/_deps/Sparkle-${METRONOM_SPARKLE_VERSION}.tar.xz")

if(NOT EXISTS "${sparkle_root}/Sparkle.framework/Versions/B/Sparkle")
    message(STATUS "Downloading Sparkle ${METRONOM_SPARKLE_VERSION}")
    file(DOWNLOAD
        "https://github.com/sparkle-project/Sparkle/releases/download/${METRONOM_SPARKLE_VERSION}/Sparkle-${METRONOM_SPARKLE_VERSION}.tar.xz"
        "${sparkle_archive}"
        EXPECTED_HASH SHA256=${METRONOM_SPARKLE_SHA256}
        TLS_VERIFY ON
        STATUS sparkle_status
    )
    list(GET sparkle_status 0 sparkle_code)
    if(NOT sparkle_code EQUAL 0)
        list(GET sparkle_status 1 sparkle_message)
        message(FATAL_ERROR "Downloading Sparkle failed: ${sparkle_message}")
    endif()
    file(REMOVE_RECURSE "${sparkle_root}")
    # tar keeps the framework's Versions/Current symlinks.
    file(MAKE_DIRECTORY "${sparkle_root}")
    execute_process(
        COMMAND tar -xf "${sparkle_archive}" -C "${sparkle_root}"
        RESULT_VARIABLE sparkle_tar
    )
    if(NOT sparkle_tar EQUAL 0)
        message(FATAL_ERROR "Extracting ${sparkle_archive} failed")
    endif()
    file(REMOVE "${sparkle_archive}")
endif()

set(SPARKLE_FRAMEWORK "${sparkle_root}/Sparkle.framework")
set(SPARKLE_BIN_DIR "${sparkle_root}/bin")
