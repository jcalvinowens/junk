.POSIX:
.SUFFIXES:
PDFLATEX = pdflatex

all: citabria.pdf decathlon.pdf da40g1000.pdf
clean:
	rm -f *.log *.aux *.pdf

.SUFFIXES: .tex .pdf
.tex.pdf:
	$(PDFLATEX) $<
