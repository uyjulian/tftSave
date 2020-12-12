
SOURCES += main.cpp

LDLIBS += -lgdi32

PROJECT_BASENAME = tftSave

RC_DESC ?= Prerendered font saver for TVP(KIRIKIRI) (2/Z)
RC_PRODUCTNAME ?= Prerendered font saver for TVP(KIRIKIRI) (2/Z)
RC_LEGALCOPYRIGHT ?= Copyright (C) 2010-2015 miahmie; Copyright (C) 2019-2020 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
