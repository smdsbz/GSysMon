GREEN		= \033[0;32m
NC		= \033[0m
PREFINFO	= $(GREEN)💬$(NC)
DIR_GUARD	= @mkdir -vp build/obj build/test

.PHONY: clean

test_cpustat: src/cpustat.c build/obj/utils.o
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building test for $< ..."
	@gcc -o build/test/$@.out -DSYSMON_CPUSTAT_TEST $^
	@echo "$(PREFINFO) Running $@ ..."
	@build/test/$@.out

build/obj/cpustat.o: src/cpustat.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

test_process_callbacks: src/process_callbacks.c build/obj/utils.o		\
		build/obj/process.o
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building test for $< ..."
	@gcc -o build/test/$@.out -DSYSMON_PROCESS_CALLBACKS_TEST $^
	@echo "$(PREFINFO) Running $@ ..."
	@build/test/$@.out

build/obj/process_callbacks.o: src/process_callbacks.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

test_process: src/process.c build/obj/utils.o
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building test for $< ..."
	@gcc -o build/test/$@.out -DSYSMON_PROCESS_TEST $^
	@echo "$(PREFINFO) Running $@ ..."
	@build/test/$@.out

build/obj/process.o: src/process.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

test_cpu: src/cpu.c build/obj/utils.o
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building test for $< ..."
	@gcc -o build/test/$@.out -DSYSMON_CPU_TEST $^
	@echo "$(PREFINFO) Running $@ ..."
	@build/test/$@.out

build/obj/cpu.o: src/cpu.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

test_system: src/system.c build/obj/utils.o
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building test for $< ..."
	@gcc -o build/test/$@.out -DSYSMON_SYSTEM_TEST $^
	@echo "$(PREFINFO) Running $@ ..."
	@build/test/$@.out

build/obj/system.o: src/system.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

build/obj/utils.o: src/utils.c
	$(DIR_GUARD)
	@echo "$(PREFINFO) Building $@ ..."
	@gcc -c -o $@ $^

clean:
	$(DIR_GUARD)
ifneq ($(wildcard build/obj/*),)
	@rm -v build/obj/*
endif
ifneq ($(wildcard build/test/*),)
	@rm -v build/test/*
endif
