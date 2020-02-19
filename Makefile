
CXX       ?= g++
CXX_FLAGS ?= -Wall -O3 -g
UDT_DIR   ?= ./udt

all: leda_vis_receiver2 test_leda_vis_receiver

leda_vis_receiver2: leda_vis_receiver2.cpp SimpleUDT.hpp DadaHeader.cc DadaHeader.h membuf.cc membuf.h $(UDT_DIR)/libudt.a
	$(CXX)      -o leda_vis_receiver2 $(CXX_FLAGS) -I$(UDT_DIR)      leda_vis_receiver2.cpp DadaHeader.cc membuf.cc -pthread $(UDT_DIR)/libudt.a

test_leda_vis_receiver: test_leda_vis_receiver.cpp SimpleUDT.hpp
	$(CXX) -o test_leda_vis_receiver $(CXX_FLAGS) -I$(UDT_DIR) test_leda_vis_receiver.cpp -pthread $(UDT_DIR)/libudt.a
