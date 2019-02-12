set(COMPONENT_SRCS
        ubirch_console.c
        cmd_system.c
        cmd_ubirch.c
        )
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES
        ubirch-esp32-networking
        ubirch-esp32-storage
        )
set(COMPONENT_PRIV_REQUIRES
        nvs_flash
        console
        )
register_component()