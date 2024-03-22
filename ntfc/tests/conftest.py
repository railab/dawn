############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

import sys
from pathlib import Path

TESTS_ROOT = Path(__file__).resolve().parent

if str(TESTS_ROOT) not in sys.path:
    sys.path.insert(0, str(TESTS_ROOT))
