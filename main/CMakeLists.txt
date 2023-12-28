set(srcs "mount.c" "server.c" "video_stream.c" "data_link.cpp" "real_stats.c" "main.c" "connect_wifi.c")
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    EMBED_FILES "template/favicon.ico" 
                    "template/upload_script.html"
                    EMBED_TXTFILES "certs/servercert.pem"
                                   "certs/prvtkey.pem")
