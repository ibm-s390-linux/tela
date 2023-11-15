#!/bin/bash
#
# Wrapper for running make in an environment without pre-defined values for
# tela-specific environment variables. This may be required to prevent
# side-effects in tests implemented as tela-based sub-Makefiles.
#

source clear_tela_vars.sh

make "${@}"
