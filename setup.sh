#!/bin/bash

echo "==================================="
echo "Security Camera SMS Relay Setup"
echo "==================================="
echo ""

# Check Python version
python_version=$(python3 --version 2>&1)
if [[ $? -ne 0 ]]; then
    echo "Error: Python 3 is not installed"
    echo "Please install Python 3.8 or higher"
    exit 1
fi
echo "✓ Found $python_version"

# Create virtual environment
echo ""
echo "Creating virtual environment..."
python3 -m venv venv
source venv/bin/activate

# Install dependencies
echo ""
echo "Installing dependencies..."
pip install --upgrade pip
pip install -r requirements.txt

# Generate default config if it doesn't exist
if [ ! -f config.yaml ]; then
    echo ""
    echo "Generating default configuration..."
    python3 -c "
from twilio_camera_relay import ConfigManager
cm = ConfigManager()
print('✓ Created config.yaml')
print('')
print('IMPORTANT: Edit config.yaml with your Twilio credentials!')
print('')
print('You need to add:')
print('  - Twilio Account SID')
print('  - Twilio Auth Token')
print('  - Twilio Phone Number')
print('  - Your mobile phone number(s)')
"
fi

# Create systemd service file
echo ""
echo "Creating systemd service file..."
cat > camera-relay.service << EOF
[Unit]
Description=Security Camera SMS Relay
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$(pwd)
Environment="PATH=$(pwd)/venv/bin"
ExecStart=$(pwd)/venv/bin/python $(pwd)/twilio_camera_relay.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

echo "✓ Created camera-relay.service"
echo ""
echo "==================================="
echo "Setup Complete!"
echo "==================================="
echo ""
echo "Next steps:"
echo ""
echo "1. Edit config.yaml with your Twilio credentials"
echo "   nano config.yaml"
echo ""
echo "2. Test the relay:"
echo "   source venv/bin/activate"
echo "   python twilio_camera_relay.py"
echo ""
echo "3. Install as a service (optional):"
echo "   sudo cp camera-relay.service /etc/systemd/system/"
echo "   sudo systemctl enable camera-relay"
echo "   sudo systemctl start camera-relay"
echo ""
echo "4. Configure your cameras to send webhooks to:"
echo "   http://$(hostname -I | awk '{print $1}'):8888/webhook"
echo ""