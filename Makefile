.PHONY: format
format:
	git ls-files '*.cpp' '*.h' | xargs clang-format -i
	black test/python

.PHONY: test-python
test-python:
	python -m unittest discover -s test/python -p "test_*.py"

.PHONY: clean
clean:
	rm -rf build/ venv valt.out