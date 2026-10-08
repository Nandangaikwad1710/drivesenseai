CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I. -DDRIVESENSE_TEST_SUITE=1

SRCS = test/test_main.cpp \
       src/can/CANManager.cpp \
       src/obd/OBDManager.cpp \
       src/obd/OBDPID.cpp \
       src/obd/OBDParser.cpp \
       src/protocol/JSONSerializer.cpp \
       src/protocol/BinarySerializer.cpp \
       src/telemetry/TelemetryManager.cpp \
       src/ble/BLEManager.cpp \
       src/system/Logger.cpp \
       src/system/SystemMonitor.cpp

TARGET = test_runner

.PHONY: all test clean

all: test

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
