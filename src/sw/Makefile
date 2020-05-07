UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	filename := Makefile-$(UNAME)
endif
ifeq ($(UNAME), Linux)
	filename := Makefile-$(UNAME)
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

