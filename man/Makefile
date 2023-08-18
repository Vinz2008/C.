
MD_FILES := $(wildcard *.md)
MAN_FILES := $(patsubst %.md, %.1, $(MD_FILES))


all: $(MAN_FILES)

%.1: %.md
	pandoc --standalone --to man $< --output $@

# pandoc --standalone --to man hello.1.md --output hello.1

clean:
	rm ./*.1

print-cpoint-run:
	groff -man -Tascii cpoint-run.1

install:
	cp ./*.1 /usr/share/man/man1