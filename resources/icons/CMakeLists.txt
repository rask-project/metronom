cmake_minimum_required(VERSION 3.19...3.24)

list(APPEND ICONS_SRC
    metronom/index.theme
    metronom/svg/quarter-note.svg
    metronom/svg/eighth-note.svg
    metronom/svg/sixteenth-note.svg
    metronom/svg/thirty-second-note.svg
    metronom/svg/pause.svg
    metronom/svg/play.svg
    metronom/svg/arrow-down-double.svg
    metronom/svg/arrow-up-double.svg
)

qt_add_resources(${TARGET_NAME} "icons"
    PREFIX "/icons"
    FILES ${ICONS_SRC}
)
