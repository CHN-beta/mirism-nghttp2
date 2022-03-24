COMMON_FLAGS = -Iinclude -I~/include -std=c++23 -Wall -Wextra -g -rdynamic -ftemplate-backtrace-limit=0 -Wl,--start-group -Wl,--no-as-needed -lfmt -lpthread -lcrypto -lssl -lbrotlicommon -lbrotlienc -lbrotlidec -lnghttp2_asio -lboost_iostreams -lbacktrace -ldl -lTgBot -lSegFault -Wl,--end-group

build:
	mkdir build

ng01: build
	g++ -o build/ng01 src/ng01.cpp -Ofast -march=native -mtune=native ${COMMON_FLAGS} ${SHARED_LINK_FLAGS}
beta: build
	g++ -o build/beta src/beta.cpp -Ofast -march=native -mtune=native ${COMMON_FLAGS} ${SHARED_LINK_FLAGS}
debug: build
	g++ -o build/debug src/debug.cpp -O0 -DMIRISM_DEBUG -fsanitize=address ${COMMON_FLAGS} ${SHARED_LINK_FLAGS}
test: build
	g++ -o build/test src/test.cpp -O0 -DMIRISM_DEBUG -fsanitize=address ${COMMON_FLAGS} ${SHARED_LINK_FLAGS}
	./build/test

clean:
	rm -rf build

.PYONE: ng01 beta debug clean