include ../tela.mak

html     := $(patsubst %.md,%.html,$(wildcard *.md))

all: pandoc $(html)
	make -C resources/ all

pandoc:
	@if ! which pandoc >/dev/null 2>&1 ; then \
		echo "Error: Need pandoc tool" >&2 ; \
		exit 1 ; \
	fi

%.html: %.md
	$(PANDOC) --from gfm --to html --standalone --metadata pagetitle="$$(head -n 1 $<)" $< --output $@
	@sed -e 's/.md"/.html"/g' -i $@

clean:
	make -C resources/ clean
	@rm -f ./*.html

.PHONY: pandoc
