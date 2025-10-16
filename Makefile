.PHONY: format
format:
	git ls-files '*.cpp' '*.h' | xargs clang-format -i

.PHONY: test-python
test-python:
	python -m unittest discover -s test/python -p "test_*.py"
