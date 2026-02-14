#!/bin/bash
# Bylins MUD Web Admin - Startup script

set -e

echo "=================================="
echo "Bylins MUD Web Admin Interface"
echo "=================================="
echo ""

# Check if MUD server is running
MUD_SOCKET="../build_debug/small/admin_api.sock"

if [ ! -S "$MUD_SOCKET" ]; then
    echo "⚠️  MUD server not running. Starting..."
    cd ../../build_debug/small
    ../circle -d . 4000 > /tmp/bylins_mud.log 2>&1 &
    MUD_PID=$!
    echo "MUD server started (PID: $MUD_PID)"
    echo "Waiting for socket..."
    sleep 5
    cd ../../tools/web_admin
    
    if [ ! -S "$MUD_SOCKET" ]; then
        echo "❌ Failed to start MUD server"
        echo "Check log: /tmp/bylins_mud.log"
        exit 1
    fi
fi

echo "✓ MUD server is running"

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
