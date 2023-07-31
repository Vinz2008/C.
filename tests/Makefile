all: python


clean:
	rm -f ./*.o ./*.ll ./*.log test ./*.temp a.out

run: all
	./test

python:
	python3 tests.py

verbose:
	python3 tests.py -v