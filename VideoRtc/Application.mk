APP_STL := gnustl_static
#APP_STL :=  stlport_shared
APP_CPPFLAGS += -fexceptions
#LOCAL_CFLAGS := -D__ANDROID__ -DHAVE_CONFIG_H -DHAVE_SYS_TIME_H
APP_CPPFLAGS += -frtti
APP_ABI := armeabi-v7a

APP_PLATFORM=android-10
