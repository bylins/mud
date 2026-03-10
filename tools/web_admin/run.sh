#!/bin/bash
# Bylins MUD Web Admin - Startup script

set -e

echo "=================================="
echo "Bylins MUD Web Admin Interface"
echo "=================================="
echo ""

MUD_SOCKET="${MUD_SOCKET:-/home/mud/mud/admin_api.sock}"

if [ ! -S "$MUD_SOCKET" ]; then
    echo "❌ MUD server socket not found: $MUD_SOCKET"
    echo "Set MUD_SOCKET env var to the correct path."
    exit 1
fi

echo "✓ MUD server socket: $MUD_SOCKET"

# Install Python dependencies if needed
if ! python3 -c "import flask" 2>/dev/null; then
    echo "Installing Python dependencies..."
    pip install -r requirements.txt
fi

echo "✓ Python dependencies installed"
echo ""
echo "Starting Flask web server..."
echo "Open browser: http://127.0.0.1:5000"
echo ""

export MUD_SOCKET
python3 app.py
