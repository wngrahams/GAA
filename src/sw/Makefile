UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	filename := Makefile-Darwin
endif
ifeq ($(UNAME), Linux)
	filename := Makefile-Linux
endif

.PHONY: default
default:
	@$(MAKE) -f $(filename)

.PHONY: clean
clean:
	@$(MAKE) -f $(filename) clean

.PHONY: all
all:
	@$(MAKE) -f $(filename) all

