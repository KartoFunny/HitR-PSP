cmake_minimum_required(VERSION 3.6.3...3.10)

project(SRR2)

# Старый код Radical 2003 года рассчитан на C++11.
# GCC 14+ в C++17+ строже по ODR и ругается на явные специализации шаблонов.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRR2_TITLE "Simpsons Hit & Run")
set(SRR2_VERSION "00.61")
set(SRR2_FILENAME "Simpsons")
set(SRR2_REGION "NTSC" CACHE STRING "Select PAL for the EFIGS multi-language version")
set_property(CACHE SRR2_REGION PROPERTY STRINGS "NTSC" "PAL")

option(SRR2_USE_DLMALLOC "Use Doug Lea's memory allocator for fixed-size heaps" OFF)
option(SRR2_REROUTE_ALL_HEAP_ALLOCATIONS "Reroute all memory allocations by new operator to internal heaps" OFF)
option(SRR2_USE_BINK "Use RAD Game Tools Bink SDK instead of FFmpeg" OFF)
option(SRR2_BUILD_TESTS "Build sample programs for internal libraries" OFF)
option(SRR2_MICROPROFILE "Use microprofile instead of the internal profiler" OFF)
option(SRR2_P3D_CG "Use CG shaders rather than GLSL for OpenGL ES" OFF)

# --- Платформа: выбор PDDI ---
if(PSP)
	set(SRR2_P3D_PDDI "GU" CACHE STRING "Pure3D PDDI implementation")
	message(STATUS "PSP detected - PDDI set to GU")
elseif(ANDROID)
	set(SRR2_P3D_PDDI "GLES2" CACHE STRING "Pure3D PDDI implementation")
	message(STATUS "Android detected - PDDI set to GLES2")
elseif(VITA)
	set(SRR2_P3D_PDDI "GXM" CACHE STRING "Pure3D PDDI implementation")
	message(STATUS "Vita detected - PDDI set to GXM")
else()
	set(SRR2_P3D_PDDI "OpenGL" CACHE STRING "Pure3D PDDI implementation")
	message(STATUS "Desktop detected - PDDI set to OpenGL")
endif()

if(PSP)
	set_property(CACHE SRR2_P3D_PDDI PROPERTY STRINGS "GU")
elseif(VITA)
	set_property(CACHE SRR2_P3D_PDDI PROPERTY STRINGS "OpenGL" "GLES1" "GLES2" "GXM")
else()
	set_property(CACHE SRR2_P3D_PDDI PROPERTY STRINGS "OpenGL" "GLES1" "GLES2")
endif()

# --- Vita ---
if(VITA)
	option(SRR2_USE_VITAGL "Use VitaGL as an OpenGL (ES) implementation" ON)
	include("${VITASDK}/share/vita.cmake" REQUIRED)

	add_definitions(-DRAD_VITA)
	set(VITA_APP_NAME ${SRR2_TITLE})
	set(VITA_TITLEID  "ROADRAGE2")
	set(VITA_VERSION ${SRR2_VERSION})
endif()

# --- Android ---
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
	add_definitions(-DRAD_ANDROID)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -llog -landroid -lEGL -lGLESv2")
endif()

# --- PSP ---
if(PSP)
	add_definitions(-DRAD_PSP)
	add_definitions(-DPSP)
	add_definitions(-DRAD_CONSOLE)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# --- SDL (только для не-PSP платформ) ---
if(NOT PSP)
	if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
		message(STATUS "Android detected - Compilando SDL2 desde libs/SDL2")
		add_subdirectory(libs/SDL2 SDL2_Build EXCLUDE_FROM_ALL)
		set(SDL_LIBRARY SDL2::SDL2)
	else()
		if(NOT VITA)
			find_package(SDL3 QUIET)
		endif()

		if(SDL3_FOUND)
			get_target_property(SDL_INCLUDE_DIR SDL3::Headers INTERFACE_INCLUDE_DIRECTORIES)
			include_directories(${SDL_INCLUDE_DIR}/SDL3)
			set(SDL_LIBRARY SDL3::SDL3)
			message(STATUS "SDL3 found in system")
		else()
			find_package(SDL2 REQUIRED)
			set(SDL_LIBRARY SDL2::SDL2 SDL2::SDL2main)
			message(STATUS "SDL2 found in system")
		endif()
	endif()
endif()

# --- libpng ---
# Сам add_subdirectory(libpng) живёт в libs/CMakeLists.txt.
# Здесь только выставляем переменные, чтобы pure3d их видел.
if(PSP OR ANDROID)
	set(PNG_LIBRARY png_static)
	set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/libpng")
else()
	find_package(PNG REQUIRED)
endif()

# --- Threads / OpenAL (не нужны на PSP) ---
if(NOT PSP)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)

	if(NOT ANDROID)
		find_package(OpenAL REQUIRED)
	endif()
endif()

# --- Опции компилятора ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-fvisibility=hidden)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		# -Werror может быть слишком строг на раннем этапе портирования; оставим его отключённым
		# add_compile_options(-Werror)
		add_compile_options(-ggdb)
	endif()
	if(VITA)
		add_compile_options(-mtune=cortex-a9 -mfpu=neon)
	endif()
		if(PSP)
		# PSP MIPS: allegrex tuning.
		# RTTI оставляем (radcontent его использует).
		# Exceptions отключены.
		# Unwind-таблицы отключены — они тянут pthread из libgcc, которого нет на PSP.
		add_compile_options(
			-O2
			-G0
			-fno-exceptions
			-fno-asynchronous-unwind-tables
			-fno-unwind-tables
			-fsingle-precision-constant
		)
	endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DRAD_DEBUG)
else()
	add_definitions(-DRAD_RELEASE)
endif()

# --- Общие defines: для PSP НЕ подключаем RAD_WIN32/RAD_SDL ---
if(PSP)
	add_definitions(-DRAD_CONSOLE)
else()
	add_definitions(-DRAD_CONSOLE -DRAD_WIN32 -DRAD_SDL)
endif()

if(SRR2_REGION STREQUAL "PAL")
	add_definitions(-DPAL)
endif()
if(SRR2_USE_DLMALLOC)
	add_definitions(-DUSE_DOUG_LEA_HEAP)
endif()
if(SRR2_REROUTE_ALL_HEAP_ALLOCATIONS)
	add_definitions(-DREROUTE_ALL_HEAP_ALLOCATIONS)
endif()

# --- FFmpeg (только для не-PSP) ---
if(SRR2_USE_BINK)
	add_definitions(-DRAD_MOVIEPLAYER_USE_BINK)
else()
	if(NOT PSP)
		if(ANDROID)
			find_package(ffmpeg CONFIG REQUIRED)
			set(SRR2_FFMPEG_LIBS
				ffmpeg::avcodec
				ffmpeg::avformat
				ffmpeg::avutil
				ffmpeg::swresample
				ffmpeg::swscale
			)
		else()
			find_package(PkgConfig REQUIRED)
			pkg_check_modules(FFmpeg REQUIRED IMPORTED_TARGET
				libavformat libavcodec libavutil libswresample libswscale)
		endif()
	endif()
endif()

if(SRR2_MICROPROFILE)
	add_definitions(-DMICROPROFILE_ENABLED)
endif()
if(SRR2_P3D_CG)
	add_definitions(-DRAD_CG)
endif()
if(SRR2_USE_VITAGL)
	add_definitions(-DRAD_VITAGL)
endif()
if(SRR2_P3D_PDDI STREQUAL "GXM")
	add_definitions(-DRAD_GXM)
endif()
if(SRR2_P3D_PDDI STREQUAL "GU")
	add_definitions(-DRAD_GU)
endif()
if(SRR2_P3D_PDDI STREQUAL "GLES1")
	add_definitions(-DRAD_GLES -DRAD_GLES_VERSION=1)
endif()
if(SRR2_P3D_PDDI STREQUAL "GLES2")
	add_definitions(-DRAD_GLES -DRAD_GLES_VERSION=2)
endif()

if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# --- Подключение библиотек и кода ---
add_subdirectory(libs)
add_subdirectory(code)

# --- Vita-упаковка ---
if(VITA)
	if(SRR2_P3D_PDDI STREQUAL "GLES2" AND NOT SRR2_USE_VITAGL)
		vita_create_self(${PROJECT_NAME}.self code/${PROJECT_NAME} UNSAFE)
		vita_create_vpk(${SRR2_FILENAME}${SRR2_REGION}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
			VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME}
			FILE sce_sys/icon0.png sce_sys/icon0.png
			sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
			sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
			sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
			art art
			module/libGLESv2.suprx module/libGLESv2.suprx
			module/libgpu_es4_ext.suprx module/libgpu_es4_ext.suprx
			module/libIMGEGL.suprx module/libIMGEGL.suprx
			module/libpvrPSP2_WSEGL.suprx module/libpvrPSP2_WSEGL.suprx
		)
	else()
		vita_create_self(${PROJECT_NAME}.self code/${PROJECT_NAME})
		vita_create_vpk(${SRR2_FILENAME}${SRR2_REGION}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
			VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME}
			FILE sce_sys/icon0.png sce_sys/icon0.png
			sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
			sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
			sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
			art art
			libs/pure3d/pddi/gxm/shaders/vertex_cg.gxp shaders/vertex_cg.gxp
			libs/pure3d/pddi/gxm/shaders/color_cg.gxp shaders/color_cg.gxp
			libs/pure3d/pddi/gxm/shaders/texture_cg.gxp shaders/texture_cg.gxp
			libs/pure3d/pddi/gxm/shaders/alpha_cg.gxp shaders/alpha_cg.gxp
		)
	endif()
endif()

# --- Switch-упаковка ---
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
	nx_generate_nacp(OUTPUT ${PROJECT_NAME}.nacp
		NAME ${SRR2_TITLE} AUTHOR "Radical Entertainment" VERSION ${SRR2_VERSION})
	nx_create_nro(code/${PROJECT_NAME}
		OUTPUT ${SRR2_FILENAME}${SRR2_REGION}.nro
		ICON ${PROJECT_NAME}.jpg NACP ${PROJECT_NAME}.nacp ROMFS romfs)
endif()
