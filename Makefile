CHARMC = /dcsdata/home/zyang/Charm/charm/bin/charmc $(OPTS)
#CHARMC = $(HOME)/collegestuff/charm/net-darwin-x86_64/bin/charmc $(OPTS)
TESTOPTS = ++local

OBJS = particle.o

N = 1000
K = 8

all: particle

particle: $(OBJS)
	$(CHARMC) -O3 -language charm++ -module CommonLBs -tracemode projections -o particle $(OBJS)

particle.decl.h: particle.ci
	$(CHARMC)  particle.ci

clean:
	rm -f *.decl.h *.def.h conv-host *.o particle charmrun

cleanp:
	rm -f *.sts *.gz *.projrc *.topo *.out

particle.o: particle.C particle.decl.h
	$(CHARMC) -c particle.C

test:
	./charmrun particle $(N) $(K) $(TESTOPTS) +balancer GreedyLB +LBDebug 1 +p4
