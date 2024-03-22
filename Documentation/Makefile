# Documentation/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#

SPHINXOPTS      ?= -j 1 -W
SPHINXAUTOOPTS  ?= -j 8 -W
SPHINXBUILD     ?= sphinx-build
SPHINXAUTOBUILD ?= sphinx-autobuild
SOURCEDIR       = .
BUILDDIR        = _build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile doxygen html autobuild clean

# Generate Doxygen XML (needed for Breathe integration)
doxygen:
	@echo "Generating Doxygen documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		(cd .. && doxygen Doxyfile > /dev/null 2>&1 && echo "Doxygen: OK") || echo "Doxygen: FAILED"; \
	else \
		echo "WARNING: Doxygen not installed. API documentation will be empty."; \
		mkdir -p _doxygen/xml _doxygen/html; \
	fi

html: doxygen
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@if [ -d "_doxygen/html" ]; then \
		mkdir -p "$(BUILDDIR)/html/doxygen"; \
		cp -a _doxygen/html/. "$(BUILDDIR)/html/doxygen/"; \
	fi

autobuild: clean
	@$(SPHINXAUTOBUILD) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXAUTOOPTS) $(O)

clean:
	@$(SPHINXBUILD) -M clean "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@rm -rf _doxygen

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
