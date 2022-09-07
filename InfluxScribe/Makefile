

BIN_DIR=/usr/local/IoT/bin
SYSTEMD_DIR=/usr/lib/systemd/system
SYSTEMD_SERVICE=IoTScribe.service


all:
	@echo "Nothing for all"

deps:
	@echo "Setting up environment for project"
	pip install -r requirements.txt

install:
	@echo "Installing project to system"
	mkdir -p $(BIN_DIR)
	cp IoTScribe.py $(BIN_DIR)/

install-service:
	cp ./IoTScribe.service $(SYSTEMD_DIR)/
	systemctl enable $(SYSTEMD_SERVICE)

status:
	systemctl status $(SYSTEMD_SERVICE)
