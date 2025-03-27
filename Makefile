TARGET := OrderBook

SRCS := $(addprefix src/, main.cpp Client.cpp OrderBook.cpp Config.cpp error.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

CCXX := g++

CXXFLAGS += -std=c++23
#warnings
CXXFLAGS += -Wall -Wextra -Werror -pedantic
#architecture
CXXFLAGS += -march=znver2 -mtune=znver2
#promises
CXXFLAGS += -fomit-frame-pointer -fno-exceptions -fno-rtti -fstrict-aliasing -fno-math-errno -fno-stack-protector
#overall settings
CXXFLAGS += -funit-at-a-time -fexpensive-optimizations -fvect-cost-model=dynamic
#math
CXXFLAGS += -freciprocal-math -fsingle-precision-constant -ffp-contract=fast
#cleanup
CXXFLAGS += -ftree-pta -ftree-copy-prop -ftree-forwprop -ftree-phiprop -ftree-scev-cprop
CXXFLAGS += -ftree-dce -ftree-builtin-call-dce -ftree-ccp -ftree-bit-ccp -ftree-dominator-opts -ftree-ch -ftree-coalesce-vars -ftree-sink -ftree-slsr -ftree-sra -ftree-pre -ftree-partial-pre -ftree-dse -ftree-vrp
CXXFLAGS += -fgcse -fgcse-lm -fgcse-sm -fgcse-las -fgcse-after-reload -fweb -flive-range-shrinkage -fsplit-wide-types -fsplit-wide-types-early 
CXXFLAGS += -fdce -fdse -fcse-follow-jumps -fcse-skip-blocks -fdelete-null-pointer-checks -fcrossjumping -fisolate-erroneous-paths-attribute
#replacements
CXXFLAGS += -ftree-vectorize -ftree-slp-vectorize -ftree-switch-conversion -ftree-tail-merge -ftree-ter
CXXFLAGS += -fdevirtualize -fdevirtualize-speculatively -fdevirtualize-at-ltrans -fstdarg-opt
CXXFLAGS += -fauto-inc-dec -fmerge-constants -fif-conversion -fif-conversion2 -fcompare-elim -fcprop-registers -fforward-propagate -fcombine-stack-adjustments -fssa-backprop -fssa-phiopt
CXXFLAGS += -foptimize-sibling-calls -foptimize-strlen
CXXFLAGS += -fipa-profile -fipa-modref -fipa-pure-const -fipa-strict-aliasing -fipa-reference -fipa-reference-addressable -fipa-vrp -fipa-cp -fipa-bit-cp -fipa-icf -fipa-sra -fipa-ra -fipa-pta -fipa-cp-clone
#loops
CXXFLAGS += -faggressive-loop-optimizations -ffinite-loops -fversion-loops-for-strides -fivopts -fmove-loop-invariants -fmove-loop-stores -ftree-loop-ivcanon -ftree-loop-linear -floop-nest-optimize -floop-block -floop-strip-mine -floop-interchange -loop-distribution -fsplit-loops -funswitch-loops -fpeel-loops -ftree-loop-if-convert -ftree-loop-im -fsplit-ivs-in-unroller -funroll-loops -floop-unroll-and-jam -ftree-loop-vectorize -fira-loop-pressure -fprefetch-loop-arrays -frerun-cse-after-loop
CXXFLAGS += -fmodulo-sched -fmodulo-sched-allow-regmoves -freschedule-modulo-scheduled-loops
#inlining
CXXFLAGS += -fearly-inlining -findirect-inlining -fpartial-inlining -finline-functions -finline-functions-called-once -finline-small-functions
#instruction reordering
CXXFLAGS += -fschedule-insns -fschedule-insns2 -fsched-interblock -fsched-spec -fselective-scheduling -fselective-scheduling2 -fsel-sched-pipelining -fsel-sched-pipelining-outer-loops
CXXFLAGS += -fshrink-wrap -fshrink-wrap-separate
CXXFLAGS += -fpeephole2 -flra-remat -fallow-store-data-races -fstore-merging -fthread-jumps -fpredictive-commoning -fsplit-paths
CXXFLAGS += -freorder-blocks -freorder-blocks-algorithm=stc -freorder-blocks-and-partition -freorder-functions -fcode-hoisting -fhoist-adjacent-loads -fira-hoist-pressure
CXXFLAGS += -falign-functions -falign-jumps -falign-labels -falign-loops
CXXFLAGS += -fcaller-saves -fdefer-pop -fguess-branch-probability
#linker
CXXFLAGS += -fno-plt -fuse-linker-plugin -flto

LDFLAGS = -static -static-libgcc -static-libstdc++

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJS) $(DEPS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(TARGET)

re: fclean $(TARGET)

.PHONY: fclean clean re
.IGNORE: fclean clean
.PRECIOUS: $(DEPS)
.SILENT: