kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kgpg/icons FILES  )

kde4_footer()



#original Makefile.am contents follow:

#kgpgiconsdir = $(kde_datadir)/kgpg/icons
#kgpgicons_ICON = AUTO
