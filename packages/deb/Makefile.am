EXTRA_DIST = changelog.in control.in rules.in

.PHONY: clean

# Building the deb packages requires the devscripts and debhelper packages.
# There are fairly standard packages for a development debian box to have
# so i dont see this as a problem.
deb: rules control changelog
	ln -sf @top_srcdir@/COPYING copyright
	ln -sf ./packages/deb @top_srcdir@/debian
	(cd @top_srcdir@; $(MAKE) -f debian/rules binary)
	touch deb

clean:
	rm -f deb

edit = sed \
	-e 's,@PACKAGE\@,$(PACKAGE),g' \
	-e 's,@PACKAGE_CONFIG_OPTS\@,$(PACKAGE_CONFIG_OPTS),g' \
	-e 's,@VERSION\@,$(VERSION),g'

changelog: changelog.in
	rm -f changelog changelog.tmp
	$(edit) $(srcdir)/changelog.in >changelog.tmp
	mv changelog.tmp changelog

control: control.in
	rm -f control control.tmp
	$(edit) $(srcdir)/control.in >control.tmp
	mv control.tmp control

rules: rules.in
	rm -f rules rules.tmp
	$(edit) $(srcdir)/rules.in >rules.tmp
	mv rules.tmp rules
