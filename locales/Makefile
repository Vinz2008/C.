LINGUAS = fr

MSGFMT=msgfmt --verbose --check
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

XGETTEXT_ARGS=--keyword=_ 

all: allmo

install:
	for t in $(LINGUAS); do cp $$t.po /usr/share/locale/$$t/LC_MESSAGES/cpoint.mo; done


%.mo: %.po
	$(MSGFMT) -o $@ $<

main.pot:
	touch $@
	(find ../src/ -name "*.h" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o main.pot)
	(find ../src -name "*.cpp" | $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o main.pot)

%.po: main.pot
	if [ -f $@ ]; then $(MSGMERGE) --previous $@ main.pot > $@.new && mv $@.new $@; else cp main.pot $@; fi

allpo:
	@-for t in $(LINGUAS); do $(MAKE) $$t.po; done

allmo:
	@for t in $(LINGUAS); do $(MAKE) $$t.mo; done

clean:
	rm main.pot