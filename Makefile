# Alternative GNU Make workspace makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

ifeq ($(config),debug)
  Boxfish_Lib_config = debug
  Boxfish_Cli_config = debug

else ifeq ($(config),release)
  Boxfish_Lib_config = release
  Boxfish_Cli_config = release

else ifeq ($(config),dist)
  Boxfish_Lib_config = dist
  Boxfish_Cli_config = dist

else
  $(error "invalid configuration $(config)")
endif

PROJECTS := Boxfish-Lib Boxfish-Cli

.PHONY: all clean help $(PROJECTS) 

all: $(PROJECTS)

Boxfish-Lib:
ifneq (,$(Boxfish_Lib_config))
	@echo "==== Building Boxfish-Lib ($(Boxfish_Lib_config)) ===="
	@${MAKE} --no-print-directory -C Boxfish-Lib -f Makefile config=$(Boxfish_Lib_config)
endif

Boxfish-Cli: Boxfish-Lib
ifneq (,$(Boxfish_Cli_config))
	@echo "==== Building Boxfish-Cli ($(Boxfish_Cli_config)) ===="
	@${MAKE} --no-print-directory -C Boxfish-Cli -f Makefile config=$(Boxfish_Cli_config)
endif

clean:
	@${MAKE} --no-print-directory -C Boxfish-Lib -f Makefile clean
	@${MAKE} --no-print-directory -C Boxfish-Cli -f Makefile clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "  debug"
	@echo "  release"
	@echo "  dist"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   Boxfish-Lib"
	@echo "   Boxfish-Cli"
	@echo ""
	@echo "For more information, see https://github.com/premake/premake-core/wiki"