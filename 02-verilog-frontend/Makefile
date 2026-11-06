CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -pedantic
YOSYS_CONFIG ?= yosys-config

COMMON_SRCS = MyParser.cpp MyModule.cpp Optimization.cpp

CONVERTER_TARGET = verilog_to_rtlil.exe
PARSER_TARGET = myverilog_parser.exe
DOT_TARGET = netlist_to_dot.exe
PLUGIN_TARGET ?= myverilog_frontend.so

ifeq ($(OS),Windows_NT)
CLEAN_CMD = powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue '$(CONVERTER_TARGET)','$(PARSER_TARGET)','$(DOT_TARGET)','$(PLUGIN_TARGET)','*.d'; exit 0"
else
CLEAN_CMD = rm -f $(CONVERTER_TARGET) $(PARSER_TARGET) $(DOT_TARGET) $(PLUGIN_TARGET) *.d
endif

.PHONY: all converter parser dot plugin clean

all: converter parser dot

converter: $(CONVERTER_TARGET)
parser: $(PARSER_TARGET)
dot: $(DOT_TARGET)

$(CONVERTER_TARGET): verilog_to_rtlil.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(PARSER_TARGET): main.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(DOT_TARGET): netlist_to_dot.cpp $(COMMON_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

plugin: myverilog_frontend.cc $(COMMON_SRCS)
	$(YOSYS_CONFIG) --build $(PLUGIN_TARGET) $^

clean:
	$(CLEAN_CMD)
