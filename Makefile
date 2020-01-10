CXXFLAGS = -W -Wall -Wextra -std=c++11

SOURCES = server.cpp

.PHONY: all clean

all: server

server: server.o
	$(CXX) $< -o $@

clean:
	rm *.o *.d server

ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
-include $(subst .cpp,.d,$(SOURCES))
endif
endif

%.d: %.cpp
	@$(CXX) -MM $(CXXFLAGS) $< > $@
