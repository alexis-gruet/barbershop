## Process this file with automake to produce Makefile.in

#bin_PROGRAMS = barbershop barbershop-client barbershop-benchmark
bin_PROGRAMS = barbershop barbershop-client
barbershop_SOURCES = barbershop.cpp barbershop.h stats.h commands.cpp commands.h store.cpp store.h Blacklist.h Blacklist.cpp
barbershop_CFLAGS = $(OPTIMIZATION) -Wall $(ARCH) $(PROF)

barbershop_client_SOURCES = client.c
barbershop_client_CFLAGS = $(OPTIMIZATION) -Wall $(ARCH) $(PROF)

#barbershop_benchmark_SOURCES = benchmark.c
#barbershop_benchmark_CFLAGS = $(OPTIMIZATION) -Wall $(ARCH) $(PROF)
