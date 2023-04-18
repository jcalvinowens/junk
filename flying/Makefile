all: citabria.pdf decathlon.pdf da40g1000.pdf

%.pdf: %.tex
	pdflatex $<

clean:
	rm -f *.log *.aux *.pdf
