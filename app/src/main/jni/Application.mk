APP_CFLAGS     := -Wall -Wextra -Wmost
APP_CFLAGS     += -fno-stack-protector -fomit-frame-pointer
APP_CONLYFLAGS := -std=c2x
APP_LDFLAGS    := -Wl,--gc-sections
APP_STL        := none

ifneq ($(NDK_DEBUG),1)
APP_CFLAGS     += -Ofast -flto -Werror
APP_CFLAGS     += -fvisibility=hidden -fvisibility-inlines-hidden
APP_CFLAGS     += -fno-unwind-tables -fno-asynchronous-unwind-tables
APP_LDFLAGS    += -flto -Wl,--exclude-libs,ALL -Wl,--strip-all
endif
