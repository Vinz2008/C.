LINGUAS = fr
#LINGUAS = fr_FR

MSGFMT=msgfmt --verbose --check
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

XGETTEXT_ARGS=--keyword=_ 

all: allmo

install:
# for t in $(LINGUAS); do cp $$t.po /usr/share/locale/$$t/LC_MESSAGES/cpoint.mo; done
# for t in $(LINGUAS); do cp $$t.po /usr/share/locale/$${t}_$${t^^}.UTF-8/LC_MESSAGES/cpoint.mo; done
	for t in $(LINGUAS); do cp cpoint/$$t.mo /usr/share/locale/$${t}/LC_MESSAGES/cpoint.mo; done
	for t in $(LINGUAS); do cp cpoint-build/$$t.mo /usr/share/locale/$${t}/LC_MESSAGES/cpoint-build.mo; done
# for t in $(LINGUAS); do cp $$t.po /usr/lib/locale/$${t}_$${t^^}.utf8/LC_MESSAGES/cpoint.mo; done

uninstall:
	for t in $(LINGUAS); do rm -f /usr/share/locale/$${t}/LC_MESSAGES/cpoint.mo; done
	for t in $(LINGUAS); do rm -f /usr/share/locale/$${t}/LC_MESSAGES/cpoint-build.mo; done

%.mo: %.po
	$(MSGFMT) -o $@ $<

cpoint/main.pot:
	touch $@
	(find ../src/ -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@)
	(find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@)

cpoint-build/main.pot:
	touch $@
	(find ../build/src -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@)
	(find ../build/src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $@)

cpoint/%.po: cpoint/main.pot
	if [ -f $@ ]; then $(MSGMERGE) --previous $@ $< > $@.new && mv $@.new $@; else cp $< $@; fi

cpoint-build/%.po: cpoint-build/main.pot
	if [ -f $@ ]; then $(MSGMERGE) --previous $@ $< > $@.new && mv $@.new $@; else cp $< $@; fi

allpo:
	@-for t in $(LINGUAS); do $(MAKE) cpoint/$$t.po; done
	@-for t in $(LINGUAS); do $(MAKE) cpoint-build/$$t.po; done

allmo:
	@for t in $(LINGUAS); do $(MAKE) cpoint/$$t.mo; done
	@for t in $(LINGUAS); do $(MAKE) cpoint-build/$$t.mo; done

clean:
	rm -f cpoint/main.pot cpoint-build/main.pot