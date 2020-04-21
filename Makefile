.PHONY: default
default:
	cd ./src && $(MAKE)

.PHONY: clean
clean:
	cd ./src && $(MAKE) clean

.PHONY: all
all:
	cd ./src && $(MAKE) all


