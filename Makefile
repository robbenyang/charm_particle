CHARMC = /home/zyang/Charm/charm/bin/charmc $(OPTS)
TESTOPTS = ++local

OBJS = particle.o

N = 10000
K = 8

all: particle

particle: $(OBJS)
	$(CHARMC) -O3 -language charm++ -module CommonLBs -module liveViz -tracemode projections -o particle $(OBJS)

particle.decl.h: particle.ci
	$(CHARMC)  particle.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o particle charmrun

cleanp: clean
	rm -f *.sts *.gz *.projrc *.topo *.out

particle.o: particle.C particle.decl.h
	$(CHARMC) -c particle.C

test:
	./charmrun particle $(N) $(K) $(TESTOPTS) +p4

test_greedy:
	./charmrun particle $(N) $(K) $(TESTOPTS) +balancer GreedyLB +LBDebug 1 +p8

test_refine:
	./charmrun particle $(N) $(K) $(TESTOPTS) +balancer RefineLB +LBDebug 1 +p8

test_refineswap:
	./charmrun particle $(N) $(K) $(TESTOPTS) +balancer RefineSwapLB +LBDebug 1 +p8

server:
	./charmrun +p8 particle $(N) $(K) $(TESTOPTS) ++server ++server-port 1234
