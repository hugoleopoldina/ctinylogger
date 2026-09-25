cmake_minimum_required(VERSION 3.15)
project(ctinylogger LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(CTINYLOGGER_BUILD_EXAMPLE "Compilar o programa de exemplo (examples/example.c)" ON)

# -----------------------------------------------------------------------
# Seleção dos arquivos de plataforma. Linux, Android e Termux usam a
# mesma implementação POSIX (pthread + ANSI); o Windows usa sua própria
# implementação nativa (WriteConsoleW), para compatibilidade correta com
# consoles antigos (Windows 7 em diante). Ver src/platform/ctl_platform.h
# para a interface comum entre as duas.
# -----------------------------------------------------------------------
if(WIN32)
    set(CTL_PLATFORM_SOURCES src/platform/windows/ctl_platform_windows.c)
else()
    set(CTL_PLATFORM_SOURCES src/platform/posix/ctl_platform_posix.c)
endif()

add_library(ctinylogger STATIC
    src/ctl_log.c
    src/ctl_config.c
    ${CTL_PLATFORM_SOURCES}
)

target_include_directories(ctinylogger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    # Silencia avisos do MSVC sobre strncpy/localtime_s/etc. serem
    # "unsafe" — usamos as variantes corretas (localtime_s) mas o MSVC
    # ainda reclama de outras chamadas padrão do C.
    target_compile_definitions(ctinylogger PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # pthreads é necessário no Linux/Android/Termux para o mutex interno.
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(ctinylogger PUBLIC Threads::Threads)
endif()

target_compile_options(ctinylogger PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
)

if(CTINYLOGGER_BUILD_EXAMPLE)
    add_executable(ctinylogger_example examples/example.c)
    target_link_libraries(ctinylogger_example PRIVATE ctinylogger)
endif()

# -----------------------------------------------------------------------
# Instalação: gera a lib estática (.a / .lib) + os headers públicos, para
# que outro projeto possa consumir ctinylogger via
# find_package/target_link_libraries ou simplesmente copiando os
# arquivos instalados.
# -----------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS ctinylogger
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ctinylogger
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
