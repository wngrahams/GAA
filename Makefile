.PHONY: default
default:
	cd ./lib && $(MAKE)
	cd ./src && $(MAKE)

.PHONY: clean
clean:
	cd ./lib && $(MAKE) clean
	cd ./src && $(MAKE) clean

.PHONY: all
all:
	cd ./lib && $(MAKE) all
	cd ./src && $(MAKE) all

