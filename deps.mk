project_root = $(CURDIR)
build ?= build

.PHONY: all capnproto

all: capnproto

capnproto:
	cd deps/capnproto; \
	cmake . -B$(build) \
	-DBUILD_SHARED_LIBS:BOOL=OFF \
	-DBUILD_TESTING:BOOL=OFF \
	-DCMAKE_BUILD_TYPE:STRING="Release" \
	-DCMAKE_INSTALL_PREFIX:PATH="$(project_root)" \
	&& cmake --build $(build) --target install

clean:
	rm -r deps/capnproto/$(build) || true
