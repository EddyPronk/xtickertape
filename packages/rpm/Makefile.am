SUBDIRS = SPECS SOURCES

RPM_DIRS = \
    BUILD \
    RPMS/i386 \
    RPMS/i586 \
    RPMS/i686 \
    RPMS/sparc \
    RPMS/alpha \
    SRPMS

.PHONY: rpmsetup

rpmsetup:
	@for DIR in $(RPM_DIRS) ; do mkdir -p $$DIR ; done
	(cd SOURCES; $(MAKE) rpm)
	(cd SPECS; $(MAKE) rpm)

rpm: rpmsetup
	rpmbuild --define "_topdir `pwd`" -ba SPECS/xtickertape.spec
