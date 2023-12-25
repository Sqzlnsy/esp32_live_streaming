set(srcs "mount.c" "server.c" "main.c" "connect_wifi.c")
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES "template/favicon.ico" "template/upload_script.html")
