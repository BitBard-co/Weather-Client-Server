# Top-level Makefile to delegate to Weather subproject

SUBDIR := Weather

all:
	$(MAKE) -C $(SUBDIR)

run:
	$(MAKE) -C $(SUBDIR) run

clean:
	$(MAKE) -C $(SUBDIR) clean

clean-all:
	$(MAKE) -C $(SUBDIR) clean-all

server:
	$(MAKE) -C $(SUBDIR) server

client:
	$(MAKE) -C $(SUBDIR) client

run-server:
	$(MAKE) -C $(SUBDIR) run-server

run-client:
	$(MAKE) -C $(SUBDIR) run-client

help:
	$(MAKE) -C $(SUBDIR) help

.PHONY: all run clean clean-all server client run-server run-client help