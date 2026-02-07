#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Test script for Admin API CRUD operations."""

import socket
import json
import sys
import os

SOCKET_PATH = "small/admin_api.sock"

def send_command(sock, command, **kwargs):
    """Send JSON command and receive response."""
    request = {"command": command, **kwargs}
    message = json.dumps(request) + "\n"
    sock.sendall(message.encode('utf-8'))

    # Read response
    buffer = b''
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Connection closed")
        buffer += chunk
        if b'\n' in buffer:
            line, buffer = buffer.split(b'\n', 1)
            return json.loads(line.decode('utf-8'))

def test_get_mob(sock, vnum):
    """Test get_mob command."""
    print(f"\n=== Testing get_mob {vnum} ===")
    response = send_command(sock, "get_mob", vnum=vnum)

    if response.get("status") == "ok":
        mob = response.get("mob", {})
        print(f"✓ Mob {vnum}: {mob.get('names', {}).get('nominative', 'N/A')}")
        print(f"  Level: {mob.get('stats', {}).get('level', 'N/A')}")
        print(f"  Aliases: {mob.get('names', {}).get('aliases', 'N/A')}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_update_mob(sock, vnum, data):
    """Test update_mob command."""
    print(f"\n=== Testing update_mob {vnum} ===")
    response = send_command(sock, "update_mob", vnum=vnum, data=data)

    if response.get("status") == "ok":
        print(f"✓ Mob {vnum} updated successfully")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_get_object(sock, vnum):
    """Test get_object command."""
    print(f"\n=== Testing get_object {vnum} ===")
    response = send_command(sock, "get_object", vnum=vnum)
    print(f"DEBUG: Full response: {response}")

    if response.get("status") == "ok":
        obj = response.get("object", {})
        print(f"✓ Object {vnum}: {obj.get('short_desc', 'N/A')}")
        print(f"  Aliases: {obj.get('aliases', 'N/A')}")
        print(f"  Type: {obj.get('type', 'N/A')}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_update_object(sock, vnum, data):
    """Test update_object command."""
    print(f"\n=== Testing update_object {vnum} ===")
    response = send_command(sock, "update_object", vnum=vnum, data=data)

    if response.get("status") == "ok":
        print(f"✓ Object {vnum} updated successfully")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def main():
    if len(sys.argv) < 2:
        print("Usage: test_admin_api.py <command> [args...]")
        print("Commands:")
        print("  get_mob <vnum>")
        print("  update_mob <vnum>")
        print("  get_object <vnum>")
        print("  update_object <vnum>")
        print("  full_crud_test")
        sys.exit(1)

    command = sys.argv[1]

    # Connect to Unix socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(SOCKET_PATH)

        # Read welcome message (sent automatically on connect)
        buffer = b''
        while b'\n' not in buffer:
            chunk = sock.recv(4096)
            if not chunk:
                raise ConnectionError("Connection closed")
            buffer += chunk
        welcome_line, _ = buffer.split(b'\n', 1)
        welcome = json.loads(welcome_line.decode('utf-8'))
        print(f"Connected: {welcome}")

        # Authenticate (using environment variables)
        username = os.getenv("MUD_USERNAME", "TestUser")
        password = os.getenv("MUD_PASSWORD", "")

        if not password:
            print("Error: MUD_PASSWORD environment variable not set")
            print("Usage: MUD_USERNAME=YourName MUD_PASSWORD=YourPassword ./test_admin_api.py <command>")
            sys.exit(1)

        auth_response = send_command(sock, "auth", username=username, password=password)
        print(f"DEBUG: Auth response: {auth_response}")
        if auth_response.get("status") != "ok":
            print(f"✗ Authentication failed: {auth_response.get('error')}")
            sys.exit(1)
        print("✓ Authenticated")

        # Execute command
        if command == "get_mob":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100
            test_get_mob(sock, vnum)

        elif command == "update_mob":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100

            # Get current mob
            mob = test_get_mob(sock, vnum)
            if mob.get("status") != "ok":
                sys.exit(1)

            # Update with test data
            data = {
                "names": {
                    "nominative": "тестовый моб (обновлен)"
                },
                "stats": {
                    "level": 42
                }
            }
            test_update_mob(sock, vnum, data)

            # Verify update
            test_get_mob(sock, vnum)

        elif command == "get_object":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100
            test_get_object(sock, vnum)

        elif command == "update_object":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100

            # Get current object
            obj = test_get_object(sock, vnum)
            if obj.get("status") != "ok":
                sys.exit(1)

            # Update with test data
            data = {
                "short_desc": "тестовый объект (обновлен)",
                "aliases": "тест объект обновлен"
            }
            test_update_object(sock, vnum, data)

            # Verify update
            test_get_object(sock, vnum)

        elif command == "full_crud_test":
            # Test mob CRUD
            print("\n" + "="*60)
            print("TESTING MOB CRUD")
            print("="*60)

            mob = test_get_mob(sock, 100)
            if mob.get("status") == "ok":
                data = {
                    "names": {
                        "nominative": "тестовый моб CRUD"
                    },
                    "stats": {
                        "level": 99
                    }
                }
                test_update_mob(sock, 100, data)
                test_get_mob(sock, 100)

            # Test object CRUD
            print("\n" + "="*60)
            print("TESTING OBJECT CRUD")
            print("="*60)

            obj = test_get_object(sock, 100)
            if obj.get("status") == "ok":
                data = {
                    "short_desc": "тестовый объект CRUD",
                    "aliases": "тест объект crud"
                }
                test_update_object(sock, 100, data)
                test_get_object(sock, 100)

        else:
            print(f"Unknown command: {command}")
            sys.exit(1)

    finally:
        sock.close()

if __name__ == "__main__":
    main()
