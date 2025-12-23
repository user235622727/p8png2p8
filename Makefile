SHELL=sh -xv
all:
	$(shell sh $(shell basename ${PWD}).c)

config:
