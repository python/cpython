all:
		@echo Nothing to make in this directory.

clean:
		find . '(' -name '*.pyc' -o -name core -o -name '*~' \
			-o -name '[@,#]*' -o -name '*.old' \
			-o -name '*.orig' -o -name '*.rej' ')' \
			-print -exec rm -f {} ';'

clobber:	clean
