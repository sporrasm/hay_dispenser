# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "horse_feeder_main.c wifi_connect.c ntp_client.c" "timer_handler.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component(SRCS "horse_feeder_main.c"
    "wifi_connect.c"
    "ntp_client.c"
    "timer_handler.c"
    INCLUDE_DIRS ".")
