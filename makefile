GREEN		= \033[0;32m
NC		= \033[0m
PREFINFO	= $(GREEN)💬$(NC)

test_process: build/obj/utils.o src/process.c
	@echo "$(PREFINFO) Building test for src/process.c ..."
	@gcc -o build/test/test_process.out -DSYSMON_PROCESS_TEST \
	    build/obj/utils.o src/process.c
	@echo "$(PREFINFO) Running test_process ..."
	@build/test/test_process.out

build/obj/process.o: build/obj/utils.o src/process.c
	@echo "$(PREFINFO) Building process.o ..."
	@gcc -c -o build/obj/process.o build/obj/utils.o src/process.c

test_cpu: build/obj/utils.o src/cpu.c
	@echo "$(PREFINFO) Building test for src/cpu.c ..."
	@gcc -o build/test/test_cpu.out -DSYSMON_CPU_TEST \
	    build/obj/utils.o src/cpu.c
	@echo "$(PREFINFO) Running test_cpu ..."
	@build/test/test_cpu.out

build/obj/cpu.o: build/obj/utils.o src/cpu.c
	@echo "$(PREFINFO) Building cpu.o ..."
	@gcc -c -o build/obj/cpu.o build/obj/utils.o src/cpu.o

test_system: build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Building test for src/system.c ..."
	@gcc -o build/test/test_system.out -DSYSMON_SYSTEM_TEST \
	    build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Running test_system ..."
	@build/test/test_system.out

build/obj/system.o: build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Building system.o ..."
	@gcc -c -o build/obj/system.o build/obj/utils.o src/system.c

build/obj/utils.o: src/utils.c
	@echo "$(PREFINFO) Building utils.o ..."
	@gcc -c -o build/obj/utils.o src/utils.c
