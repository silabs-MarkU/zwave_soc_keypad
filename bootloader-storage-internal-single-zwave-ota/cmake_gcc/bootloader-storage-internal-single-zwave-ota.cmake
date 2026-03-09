####################################################################
# Automatically-generated file. Do not edit!                       #
####################################################################

set(SDK_PATH "/Users/maumina/.silabs/slt/installs/conan/p/simpl1a11563c2e399/p")
set(COPIED_SDK_PATH "simplicity_sdk_2025.12.1")
set(PKG_PATH "/Users/maumina/.silabs/slt/installs")

add_library(slc OBJECT
    "${SDK_PATH}/bootloader/platform/bootloader/app_version/btl_parser_app_version_check.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_bootload.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_core.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_main.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_parse.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/btl_reset.c"
    "${SDK_PATH}/bootloader/platform/bootloader/core/flash/btl_internal_flash.c"
    "${SDK_PATH}/bootloader/platform/bootloader/debug/btl_debug.c"
    "${SDK_PATH}/bootloader/platform/bootloader/debug/btl_debug_swo.c"
    "${SDK_PATH}/bootloader/platform/bootloader/driver/btl_driver_util.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression/btl_decompress_lzma.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression/lzma/LzmaDec.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_custom_tags.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_format.c"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/gbl/btl_gbl_parser.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_crc16.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_crc32.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_aes.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_ecdsa.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_sha256.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/btl_security_tokens.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/ecc/ecc.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/btl_sha256.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/crypto_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/cryptoacc_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/security/sha/se_sha.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/bootloadinfo/btl_storage_bootloadinfo.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/btl_storage.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/btl_storage_library.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/internal_flash/btl_storage_internal_flash.c"
    "${SDK_PATH}/bootloader/platform/bootloader/storage/internal_flash/btl_storage_internal_flash_raw.c"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32ZG23/Source/startup_efr32zg23.c"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32ZG23/Source/system_efr32zg23.c"
    "${SDK_PATH}/platform_common/platform/common/src/sl_assert.c"
    "${SDK_PATH}/platform_common/platform/common/src/sl_syscalls.c"
    "${SDK_PATH}/platform_core/platform/common/src/sl_core_cortexm.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_acmp.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_burtc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_cmu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_dbg.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_emu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_eusart.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_gpcrc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_gpio.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_i2c.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_iadc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_lcd.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_ldma.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_lesense.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_letimer.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_msc.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_opamp.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_pcnt.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_prs.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_rmu.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_system.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_timer.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_usart.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_vdac.c"
    "${SDK_PATH}/platform_core/platform/emlib/src/em_wdog.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_dynamic_reservation.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_pool.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_pool_common.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_region.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sl_memory_manager_retarget.c"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src/sli_memory_manager_common.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/src/se_aes.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/src/sl_mbedtls.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_psa_driver_common.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_psa_driver_init.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_builtin_keys.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_key_management.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_driver_signature.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_opaque_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_aead.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_cipher.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_hash.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_driver_mac.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_transparent_key_derivation.c"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/src/sli_se_version_dependencies.c"
    "${SDK_PATH}/security_mbedtls_source/library/aes.c"
    "${SDK_PATH}/security_mbedtls_source/library/constant_time.c"
    "${SDK_PATH}/security_mbedtls_source/library/platform.c"
    "${SDK_PATH}/security_mbedtls_source/library/platform_util.c"
    "${SDK_PATH}/security_mbedtls_source/library/psa_crypto_client.c"
    "${SDK_PATH}/security_mbedtls_source/library/psa_util.c"
    "${SDK_PATH}/security_mbedtls_source/library/threading.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_attestation.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_cipher.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_entropy.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_hash.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_key_derivation.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_key_handling.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_signature.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sl_se_manager_util.c"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/src/sli_se_manager_mailbox.c"
    "${SDK_PATH}/security_tfm/lib/fih/src/fih.c"
    "${SDK_PATH}/security_tfm/platform/ext/target/siliconlabs/hse/sli_se.c"
)

target_include_directories(slc PUBLIC
   "../config"
   "../autogen"
    "${SDK_PATH}/devices/platform/Device/SiliconLabs/EFR32ZG23/Include"
    "${SDK_PATH}/platform_common/platform/common/inc"
    "${SDK_PATH}/bootloader/platform/bootloader"
    "${SDK_PATH}/bootloader/platform/bootloader/api"
    "${SDK_PATH}/bootloader/platform/bootloader/parser/compression"
    "${SDK_PATH}/bootloader/platform/bootloader/debug"
    "${SDK_PATH}/bootloader/platform/bootloader/parser"
    "${SDK_PATH}/bootloader/platform/bootloader/core/flash"
    "${SDK_PATH}/bootloader/platform/bootloader/security"
    "${SDK_PATH}/cmsis/Core/Include"
    "${SDK_PATH}/platform_core/platform/emlib/inc"
    "${SDK_PATH}/platform_core/platform/common/errno_error_codes/inc"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/config"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/config/preset"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_mbedtls_support/inc"
    "${SDK_PATH}/security_mbedtls_source/include"
    "${SDK_PATH}/security_mbedtls_source/library"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/inc"
    "${SDK_PATH}/platform_core/platform/service/memory_manager/src"
    "${SDK_PATH}/security_mbedtls/platform/security/sl_component/sl_psa_driver/inc"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/se_manager/inc"
    "${SDK_PATH}/platform_core/platform/common/inc"
    "${SDK_PATH}/security_tfm/lib/fih/inc"
    "${SDK_PATH}/security_tfm/platform/include"
    "${SDK_PATH}/security_se_manager/platform/security/sl_component/sli_psec_osal/inc"
)

target_compile_definitions(slc PUBLIC
    "EFR32ZG23B020F512IM48=1"
    "SL_CODE_COMPONENT_SYSTEM=system"
    "SE_MANAGER_CONFIG_FILE=\"btl_aes_ctr_stream_block_cfg.h\""
    "PARSER_HAS_APPLICATION_UPGRADE_VALIDATION_CALLBACK=1"
    "BTL_PARSER_SUPPORT_CUSTOM_TAGS=1"
    "BTL_PARSER_SUPPORT_LZMA=1"
    "_LZMA_SIZE_OPT=1"
    "BOOTLOADER_ENABLE=1"
    "BOOTLOADER_SECOND_STAGE=1"
    "SL_RAMFUNC_DISABLE=1"
    "__START=main"
    "__STARTUP_CLEAR_BSS=1"
    "SYSTEM_NO_STATIC_MEMORY=1"
    "BOOTLOADER_SUPPORT_INTERNAL_STORAGE=1"
    "BOOTLOADER_SUPPORT_STORAGE=1"
    "SL_BOARD_NAME=\"BRD4210A\""
    "SL_BOARD_REV=\"A01\""
    "HARDWARE_BOARD_DEFAULT_RF_BAND_868=1"
    "HARDWARE_BOARD_SUPPORTS_2_RF_BANDS=1"
    "HARDWARE_BOARD_SUPPORTS_RF_BAND_868=1"
    "HARDWARE_BOARD_SUPPORTS_RF_BAND_915=1"
    "HFXO_FREQ=39000000"
    "SL_COMPONENT_CATALOG_PRESENT=1"
    "MBEDTLS_CONFIG_FILE=<sl_mbedtls_trustzone_config.h>"
    "SL_CODE_COMPONENT_MEMORY_MANAGER=memory_manager"
    "MBEDTLS_PSA_CRYPTO_CONFIG_FILE=<psa_crypto_config.h>"
    "SL_CODE_COMPONENT_SE_MANAGER=se_manager"
    "SL_CODE_COMPONENT_CORE=core"
    "SL_CODE_COMPONENT_PSEC_OSAL=psec_osal"
    "SL_TRUSTZONE_SECURE=1"
)

target_link_libraries(slc PUBLIC
    "-Wl,--start-group"
    "gcc"
    "c"
    "m"
    "nosys"
    "-Wl,--end-group"
)
target_compile_options(slc PUBLIC
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:C>:-mthumb>
    $<$<COMPILE_LANGUAGE:C>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:C>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:C>:-mcmse>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Os>
    $<$<COMPILE_LANGUAGE:C>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:C>:-g>
    $<$<COMPILE_LANGUAGE:C>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:C>:-Wno-ignored-qualifiers>
    $<$<COMPILE_LANGUAGE:C>:-Wno-sign-compare>
    $<$<COMPILE_LANGUAGE:C>:-fno-lto>
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:CXX>:-mthumb>
    $<$<COMPILE_LANGUAGE:CXX>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:CXX>:-mfloat-abi=hard>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-mcmse>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Os>
    $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:CXX>:-fomit-frame-pointer>
    $<$<COMPILE_LANGUAGE:CXX>:-g>
    $<$<COMPILE_LANGUAGE:CXX>:--specs=nano.specs>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-ignored-qualifiers>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-compare>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-lto>
    $<$<COMPILE_LANGUAGE:ASM>:-mcpu=cortex-m33>
    $<$<COMPILE_LANGUAGE:ASM>:-mthumb>
    $<$<COMPILE_LANGUAGE:ASM>:-mfpu=fpv5-sp-d16>
    $<$<COMPILE_LANGUAGE:ASM>:-mfloat-abi=hard>
    "$<$<COMPILE_LANGUAGE:ASM>:SHELL:-x assembler-with-cpp>"
)

set(post_build_command ${POST_BUILD_EXE} postbuild "./bootloader-storage-internal-single-zwave-ota.slpb" --parameter build_dir:"$<TARGET_FILE_DIR:bootloader-storage-internal-single-zwave-ota>")
set_property(TARGET slc PROPERTY C_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_STANDARD 17)
set_property(TARGET slc PROPERTY CXX_EXTENSIONS OFF)

target_link_options(slc INTERFACE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -T${CMAKE_CURRENT_LIST_DIR}/../autogen/linkerfile.ld
    --specs=nano.specs
    "SHELL:-Xlinker -Map=$<TARGET_FILE_DIR:bootloader-storage-internal-single-zwave-ota>/bootloader-storage-internal-single-zwave-ota.map"
    "SHELL:-Wl,--wrap=_free_r -Wl,--wrap=_malloc_r -Wl,--wrap=_calloc_r -Wl,--wrap=_realloc_r"
    -fno-lto
    -Wl,--gc-sections
)

# BEGIN_SIMPLICITY_STUDIO_METADATA=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END_SIMPLICITY_STUDIO_METADATA