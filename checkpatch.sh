./scripts/checkpatch.pl -f -q --no-tree include/*.h \
					include/*/*.h \
					include/arch/i386/*.h \
					boot/* \
					kernel/* \
					lib/* \
					user/* \
					fs/* \

./scripts/checkpatch.pl -f -q --no-tree Makefile \

./scripts/checkpatch.pl -f -q --no-tree net/Makefile \
					net/*.[cSh] \
