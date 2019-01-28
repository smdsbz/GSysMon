GREEN		= \033[0;32m
NC		= \033[0m
PREFINFO	= $(GREEN)💬$(NC)


test_system: build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Building test for src/system.c ..."
	@gcc -o build/test/test_system.out -DSYSMON_SYSTEM_TEST build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Running test_system ..."
	@build/test/test_system.out

build/obj/system.o: build/obj/utils.o src/system.c
	@echo "$(PREFINFO) Building system.o ..."
	@gcc -c -o build/obj/system.o build/obj/utils.o src/system.c

build/obj/utils.o: src/utils.c
	@echo "$(PREFINFO) Building utils.o ..."
	@gcc -c -o build/obj/utils.o src/utils.c
